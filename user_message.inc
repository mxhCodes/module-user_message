<?php

/**
 * @file
 *  user_message entity and controller class file. 
 */

class UserMessageEntityController extends EntityAPIController {
  
  public function create(array $values = array()) {
    global $user;
   
    if (!isset($values['subject'])) {
      $values['subject'] = '';
    }
    if (!isset($values['created_by'])) {
      $values['created_by'] = $user->uid;
    }
    if (!isset($values['created_date'])) {
      $values['created_date'] = REQUEST_TIME;
    }
    if (!isset($values['stamp'])) {
      $values['stamp'] = REQUEST_TIME;
    }
    if (!isset($values['mail_status']) || !$values['mail_status']) {
      $values['mail_status'] = 'no_mail';
    }
    if (!isset($values['status'])) {
      $values['status'] = 'draft';
    }
    if (!isset($values['type'])) {
      $values['type'] = 'common';
    }
    if (!isset($values['attached_data'])) {
      $values['attached_data'] = array();
    }
    
    return parent::create($values);
  }
  
  public function load($ids = array(), $conditions = array()) {
    $ms = parent::load($ids, $conditions);
    foreach ($ms as $m) {
      // Unserialize attached data.
      $m->attachedData();
    }
    return $ms;
  }
}

class UserMessageEntity extends Entity {
  
  protected $prepared;
  protected $finished;
  protected $read_status;
  protected $recipients;
  protected $received_items_all; // All received items.
  protected $received_item; // Certainly loaded items.
  protected $proper_received_item;
  protected $replies;
  protected $archived;
  
  public function __construct(array $values = array(), $entityType = NULL) {
    $this->prepared = FALSE;
    $this->finished = FALSE;
    $this->archived = array();
    parent::__construct($values, $entityType);
  }
  
  function defaultUri() {
    return array('path' => 'user-message/' . $this->umid);
  }
  
  function defaultLabel() {
    return $this->subject;
  }
  
  /**
   * Retrieve attached data (unserialized).
   */
  function attachedData() {
    if (isset($this->attached_data) && is_string($this->attached_data)) {
      // Attached data will be serialized during saving process,
      // @see UserMessageEntity::save().
      $this->attached_data = unserialize($this->attached_data);
    }
    return $this->attached_data;
  }
  
  function save() {
    if (isset($this->attached_data) && !is_string($this->attached_data)) {
      // Make sure attached data is serialized before it's getting saved.
      $this->attached_data = serialize($this->attached_data);
    }
    if (!$this->mail_status) {
      // Convert FALSE to 'no_mail'.
      $this->mail_status = 'no_mail';
    }
    return parent::save();
  }
  
  function repliedMessage() {
    if (isset($this->reply_to) && !empty($this->reply_to)) {
      return user_message_load($this->reply_to);
    }
    return FALSE;
  }
  
  function replies() {
    if (isset($this->replies)) {
      return $this->replies;
    }
    
    $query = user_message_build_query();
    $query->where('user_message.reply_to = ' . $this->umid);
    $umids = $query->execute()->fetchCol();
    
    $this->replies = $umids ? user_message_load_multiple($umids) : array();
    return $this->replies();
  }
  
  function rootMessage() {
    if ($replied = $this->repliedMessage()) {
      return $replied->rootMessage();
    }
    else {
      return $this;
    }
  }
  
  function recipients() {
    if (isset($this->recipients)) {
      return $this->recipients;
    }
    $this->recipients = module_invoke_all('user_message_recipients', $this);
    return $this->recipients();
  }
  
  function prepareTransmission() {
    $this->status = 'transmission';
    $this->prepared = $this->save() ? TRUE : FALSE;
    return $this->prepared;
  }
  
  function performTransmission() {
    if (!$this->prepared) {
      // Cant transmit unprepared messages.
      return $this->finished;
    }
    
    $recipients = $this->recipients();
    $alterable = array('user_message' => $this, 'recipients' => &$recipients);
    drupal_alter('user_message_transmit', $alterable);
    
    foreach ($recipients as $groupkey => $group) {
      foreach ($group['items'] as $delta => $recipient) {
        $values = array(
          'umid' => $this->umid,
          'groupkey' => $groupkey,
          'delta' => $delta,
          'label' => $recipient['label'],
          'path' => isset($recipient['path']) ? $recipient['path'] : NULL,
          'entity_id' => NULL,
          'entity_type' => NULL,
        );
        if (isset($recipient['entity_id'], $recipient['entity_type'])) {
          $values['entity_id'] = $recipient['entity_id'];
          $values['entity_type'] = $recipient['entity_type'];
        }
        $received = (object) $values;
        $this->saveReceivedItem($received);
        
        if ($this->mailStatus() == 'send_mail' && isset($recipient['emails'])) {
          foreach ($recipient['emails'] as $mail) {
            $record = array(
              'umid' => $this->umid,
              'groupkey' => $groupkey,
              'delta' => $delta,
              'email' => $mail,
              'mail_sent' => 0, // Sent flag will be changed at the mail-send operation (see finishTransmission()).
            );
            $this->saveMail($record);
          }
        }
      }
    }
    
    $this->prepared = FALSE;
    
    $this->finished = $this->mailStatus() == 'send_mail' ? FALSE : $this->finishTransmission();
    
    return $this->finished;
  }
  
  function finishTransmission($batch = FALSE, $limit = NULL) {
    if ($this->prepared || $this->finished) {
      // No finishing process required.
      // Either the user_message has been prepared for performing transmission,
      // or the transmission process has already been completed.
      return $this->finished;
    }
    
    $batch_completed = FALSE;
    if ($batch && !isset($limit)) {
      // Do we need a batch operation?
      if ($this->countPendingMails()) {
        $batch = array(
          'title' => 'Die E-Mails werden versandt.',
          'operations' => array(
            array('user_message_mail_batch_operation', array($this->umid)),
          ),
          'finished' => 'user_message_mail_batch_finished',
        );
        batch_set($batch);
      }
      else {
        // Batch operation is not required.
        return $this->finishTransmission();
      }
    }
    elseif ($this->mail_status == 'send_mail') {
      $batch_completed = $this->sendMail($limit);
    }
    
    if ($batch_completed) {
      $this->mail_status = 'mail_sent';
    }
    
    if (!$batch || $batch_completed) {
      $this->status = 'transmitted';
      $this->stamp = REQUEST_TIME;
      $this->finished = $this->save() ? TRUE : FALSE;
    }
    
    return $this->finished;
  }
  
  function mailStatus() {
    return $this->mail_status;
  }
  
  function countPendingMails() {
    $query = $this->buildMailQuery();
    $query->where('mail.mail_sent = 0');
    
    $query = $query->countQuery();
    $count = $query->execute()->fetchField();
    
    return $count;
  }
  
  /**
   * Get all or a part of received items which belong to the user message.
   * 
   * You may track down the items to a single recipient group
   * by using the $groupkey parameter only, or if you want to
   * retrieve a single item use both $groupkey and $delta.
   * 
   * @param $groupkey
   *  (Optional) The groupkey to track down.
   * @param $delta
   *  (Optional) The delta info key.
   *  If set, $groupkey is also required.
   * 
   * @return
   *  Either an array of recipient groups when using no parameter,
   *  or an array of group items when using $groupkey only,
   *  or a single received item object when using both $groupkey and $delta.
   */
  function receivedItems($groupkey = NULL, $delta = NULL) {
    if (isset($this->received_items_all) && !isset($groupkey)) {
      return $this->received_items_all;
    }
    elseif (isset($groupkey, $delta) && isset($this->received_item[$groupkey][$delta])) {
      return $this->received_item[$groupkey][$delta];
    }
    elseif (isset($groupkey) && !isset($delta) && isset($this->received_item[$groupkey])) {
      return $this->received_item[$groupkey];
    }
    
    $query = $this->buildReceivedItemQuery();
    
    if (isset($groupkey)) {
      $query->where('received.groupkey = :groupkey', array(':groupkey' => $groupkey));
      if (isset($delta)) {
        $query->where('received.delta = :delta', array(':delta' => $delta));
      }
    }
    
    $rows = $query->execute()->fetchAll();
    
    foreach ($rows as $item) {
      if (!isset($groupkey)) {
        $this->received_items_all[$item->groupkey][$item->delta] = $item;
      }
      $this->received_item[$item->groupkey][$item->delta] = $item;
    }
    
    if (isset($groupkey, $delta) && !isset($this->received_item[$groupkey][$delta])) {
      $this->received_item[$groupkey][$delta] = FALSE;
    }
    elseif (isset($groupkey) && !isset($this->received_item[$groupkey])) {
      $this->received_item[$groupkey] = array();
    }
    
    return $this->receivedItems($groupkey, $delta);
  }
  
  /**
   * Try to find the proper received item for the current user account. 
   * 
   * @return
   *  The user_message_received object which is the proper received item for the account,
   *  FALSE if no proper received item was found.
   */
  function properReceivedItem($account = NULL) {
    global $user;
    if (!isset($account)) {
      $account = user_load($user->uid);
    }
    
    if (isset($this->proper_received_item[$account->uid])) {
      return $this->proper_received_item[$account->uid];
    }
    
    $query = $this->buildReceivedItemQuery();
    $query->where('received.entity_type = :type', array(':type' => 'user'));
    $query->where('received.entity_id = :uid', array(':uid' => $account->uid));
    $received = $query->execute()->fetch();
    
    $this->proper_received_item[$account->uid] = $received ? $received : FALSE;
    
    $invoke = module_invoke_all('user_message_proper_received_item', $this, $account, $this->proper_received_item[$account->uid]);
    if (!empty($invoke)) {
      $invoke = array_reverse($invoke);
      $this->proper_received_item[$account->uid] = reset($invoke);
    }
    
    return $this->properReceivedItem($account);
  }
  
  /**
   * Marks the user message as 'read'.
   */
  function read($entity = NULL, $entity_type = 'user') {
    global $user;
    if (!isset($entity) && $entity_type == 'user') {
      $entity = user_load($user->uid);
    }
    
    if ($entity_type == 'user') {
      $received = $this->properReceivedItem($entity);
      if (is_object($received) && UserMessageReceived::recipientEntity($received)) {
        // User may read this message because he received it.
        $entity = UserMessageReceived::recipientEntity($received);
        $entity_type = $received->entity_type;
      }
      elseif ($entity->uid == $this->created_by) {
        // User may read this message because he created it.
      }
      else {
        // Check whether the user is actually allowed to read this message.
        if (!user_message_access('view', $this, $entity)) {
          throw new Exception('Unallowed attempt to read a user message by user ' . $entity->name);
        }
      }
    }
    
    module_invoke_all('user_message_read', $this, $entity, $entity_type);
    
    $read_status = $this->readStatus($entity, $entity_type);
    
    if (!$read_status) {
      // To mark the user_message as read by the entity,
      // an item is being throwed in the read pool for this.
      $item = pool_item_create($this, 'user_message', $entity, $entity_type);
      pool_throwin('read', $item);
      // Update the read status.
      $id = entity_id($entity_type, $entity);
      $this->read_status[$entity_type][$id] = $item->stamp;
    }
  }

  /**
   * Archive the user message.
   * 
   * @return bool
   *  TRUE if archiving was succesful, FALSE otherwise.
   */
  function archive($entity = NULL, $entity_type = 'user') {
    global $user;
    if (!isset($entity) && $entity_type == 'user') {
      $entity = user_load($user->uid);
    }

    if ($this->status != 'transmitted') {
      // Only allow archiving for transmitted messages.
      return FALSE;
    }

    module_invoke_all('user_message_archive', $this, $entity, $entity_type);
    
    // @todo Does it make sense to read the user message before archiving?
    $this->read($entity, $entity_type);
    
    $read_status = $this->readStatus($entity, $entity_type);
    if ($read_status) {
      $item = pool_item_create($this, 'user_message', $entity, $entity_type);
      pool_throwin('archive', $item);
      return TRUE;
    }

    return FALSE;
  }
  
  /**
   * Un-archive the user message.
   * 
   * @return bool
   *  TRUE if un-archiving was succesful, FALSE otherwise. 
   */
  function unarchive($entity = NULL, $entity_type = 'user') {
    global $user;
    if (!isset($entity) && $entity_type == 'user') {
      $entity = user_load($user->uid);
    }

    $item = pool_item_create($this, 'user_message', $entity, $entity_type);
    pool_pullout('archive', $item, FALSE);

    return TRUE;
  }

  /**
   * Whether the user message has been archived by the given entity.
   * 
   * @return
   *  TRUE if the user message has been archived, FALSE otherwise.
   */
  function isArchived($entity = NULL, $entity_type = 'user') {
    global $user;
    if (!isset($entity) && $entity_type == 'user') {
      $entity = user_load($user->uid);
    }

    $id = entity_id($entity_type, $entity);
    if (isset($this->archived[$entity_type][$id])) {
      return $this->archived[$entity_type][$id];
    }

    $item = pool_item_create($this, 'user_message', $entity, $entity_type);
    $this->archived[$entity_type][$id] = pool_pullout('archive', $item) ? TRUE : FALSE;
    return $this->archived[$entity_type][$id];
  }

  /**
   * @param $entity
   * @param $entity_type
   * 
   * @return
   * FALSE if entity has not read the message yet,
   * otherwise a timestamp of first read will be returned. 
   */
  function readStatus($entity = NULL, $entity_type = 'user') {
    global $user;
    if (!isset($entity) && $entity_type == 'user') {
      $entity = user_load($user->uid);
    }

    if ($entity_type == 'user') {
      $received = $this->properReceivedItem($entity);
      if (is_object($received) && UserMessageReceived::recipientEntity($received)) {
        $entity = UserMessageReceived::recipientEntity($received);
        $entity_type = $received->entity_type;
      }
    }
    
    $extract = entity_extract_ids($entity_type, $entity);
    $id = $extract[0];
    
    if (isset($this->read_status[$entity_type][$id])) {
      return $this->read_status[$entity_type][$id];
    }
    
    $item = pool_item_create($this, 'user_message', $entity, $entity_type);
    $item = pool_pullout('read', $item);
    $this->read_status[$entity_type][$id] = $item ? $item->stamp : FALSE;
    
    return $this->readStatus($entity, $entity_type);
  }
  
  protected function sendMail($limit = NULL) {
    $query = $this->buildMailQuery($limit);
    $query->where('mail.mail_sent = 0');
    
    $records = $query->execute()->fetchAllAssoc('email');
    
    if ($records) {
      foreach ($records as $email => $record) {
        drupal_mail('user_message', 'user_message', $email, language_default(), array('user_message' => $this));
        $record->mail_sent = 1;
        $this->saveMail($record);
      }
    }
    
    $query->range(0, 1);
    $existing = $query->execute()->fetchField();
    
    return !$existing ? TRUE : FALSE;
  }
  
  /**
   * Saves (inserts or updates) a mail record.
   */  
  protected function saveMail(&$record) {
    $object = (object) $record;
    
    $query = $this->buildMailQuery(1);
    $query->where('mail.email = :email', array(':email' => $object->email));
    
    $existing = $query->execute()->fetch();
    if ($existing) {
      // Keep group and delta info on existing mail records.
      $object->groupkey = $existing->groupkey;
      $object->delta = $existing->delta;
      drupal_write_record('user_message_mail', $object, array('umid', 'email'));
    }
    else {
      drupal_write_record('user_message_mail', $object);
    }
    
    if (is_array($record)) {
      $record = (array) $object;
    }
  }
  
  /**
   * Saves (inserts or updates) a received item.
   */
  protected function saveReceivedItem($item) {
    $existing = $this->receivedItems($item->groupkey, $item->delta);
    if ($existing) {
      // Keep stamp info on existing received items.
      $item->stamp = $existing->stamp;
      drupal_write_record('user_message_received', $item, array('umid', 'groupkey', 'delta'));
    }
    else {
      $item->stamp = REQUEST_TIME;
      drupal_write_record('user_message_received', $item);
    }
  }
  
  protected function buildReceivedItemQuery() {
    $query = db_select('user_message_received', 'received')
      ->fields('received')
      ->where('received.umid = :umid', array(':umid' => $this->umid))
      ->orderBy('received.groupkey', 'ASC')
      ->orderBy('received.delta', 'ASC');
    
    return $query;
  }
  
  protected function buildMailQuery($limit = NULL) {
    $query = db_select('user_message_mail', 'mail')
      ->fields('mail')
      ->where('mail.umid = :umid', array(':umid' => $this->umid))
      ->orderBy('mail.email', 'ASC');
    
    if (isset($limit)) {
      $query->range(0, $limit);
    }
    
    return $query;
  }
}
