<?php

/**
 * @file
 * 
 * Helper functions for user_message received items.
 */

class UserMessageReceived {
  
  /**
   * Get the recipient's uri by given received item. 
   */
  static function recipientUri($item) {
    $uri = isset($item->path) && $item->path ? array('path' => $item->path) : FALSE;

    // Try to get path by recipient's entity, if available.
    if (!$uri && ($entity = UserMessageReceived::recipientEntity($item))) {
      $uri = is_object($entity) ? entity_uri($item->entity_type, $entity) : FALSE;
    }

    return $uri ? $uri : NULL;
  }
  
  /**
   * Get the label of the recipient by given received item. 
   */
  static function recipientLabel($item) {
    $label = check_plain($item->label);
    
    // Try to get label by recipient's entity, if available.
    if ($entity = UserMessageReceived::recipientEntity($item)) {
      $entitylabel = entity_label($item->entity_type, $entity);
      if ($entitylabel != $label) {
        $label = $entitylabel . ' (' . t('Was: @oldlabel', array('@oldlabel' => $label)) . ')';
      }
    }
    
    return $label;
  }
  
  /**
   * Get the recipient as entity by given received item, if available.
   * 
   * @return
   *  The recipient as entity, FALSE otherwise. 
   */
  static function recipientEntity($item) {
    if (isset($item->entity_id, $item->entity_type) && $item->entity_type) {
      $entity = entity_load($item->entity_type, array($item->entity_id));
      $recipient = !empty($entity) ? reset($entity) : FALSE;
    }
    else {
      $recipient = FALSE;
    }

    return $recipient;
  }

  static function sentMails($item) {
    $query = db_select('user_message_mail', 'mail');
    $query->where('mail.umid = :umid', array(':umid' => $item->umid))
      ->where('mail.groupkey = :groupkey', array(':groupkey' => $item->groupkey))
      ->where('mail.delta = :delta', array(':delta' => $item->delta))
      ->where('mail.mail_sent = 1');
    $query->fields('mail', array('email'));
    $query->orderBy('mail.email', 'ASC');

    $mails = $query->execute()->fetchCol();

    return $mails;
  }

}
