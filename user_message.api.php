<?php

/**
 * @file
 * API documentation for User Message.
 */

/**
 * Allows to extend or manipulate the query for fetching transmitted messages.
 * 
 * @param $account
 *  The user account object to fetch the transmitted messages for.
 * @param $query
 *  The current query build.
 * 
 * @return
 *  Nothing to return. The query object itself is being manipulated by reference.
 */
function hook_user_message_build_transmitted_query($account, $query) {
  // Only fetch common user messages.
  $query->where('user_message.type = :type', array(':type' => 'common'));
}

/**
 * Allows to extend or manipulate the query for fetching received messages.
 * 
 * @param $account
 *  The user account object to fetch the received messages for.
 * @param $query
 *  The current query build.
 * 
 * @return
 *  Nothing to return. The query object itself is being manipulated by reference.
 */
function hook_user_message_build_received_query($account, $query) {
  // Only fetch common user messages.
  $query->where('user_message.type = :type', array(':type' => 'common'));
}

/**
 * Allows to extend or manipulate the query for fetching a user's draft messages.
 * 
 * @param $account
 *  The user account object to fetch the draft messages for.
 * @param $query
 *  The current query build.
 * 
 * @return
 *  Nothing to return. The query object itself is being manipulated by reference.
 */
function hook_user_message_build_draft_query($account, $query) {
  // Only fetch common user messages.
  $query->where('user_message.type = :type', array(':type' => 'common'));
}

/**
 * Determine user access for user_message objects.
 * 
 * @return
 *  USER_MESSAGE_ACCESS_IGNORE / USER_MESSAGE_ACCESS_ALLOW / USER_MESSAGE_ACCESS_DENY
 */
function hook_user_message_access($op, $m = NULL, $account) {
  if ($op == 'view') {
    // Always grant access on view operations, unless
    // no other module has denied the access.
    return USER_MESSAGE_ACCESS_ALLOW;
  }
  return USER_MESSAGE_ACCESS_IGNORE; // Dont change access permissions.
}

/**
 * Determine recipients for a given user_message object.
 * 
 * @param $m
 *  The user_message object to get the recipients for.
 * 
 * @return
 *  An associative array, whose keys are the group of the recipient list.
 *  The keys of the group are the following:
 *    - 'group label': A translated string of the name of the group.
 *    - 'items': An array of recipient items which belong to the group.
 *       You may use the key of the item as delta information.
 *       An item itself has the following keys:
 *      - 'label': A translated string of the item's label / name.
 *      - 'emails': (Optional) An array of email addresses.
 *      - 'path': (Optional) If defined, a link will be generated for this item.
 *      - 'entity_id': (Optional) When the recipient is an entity, use this key for the entity id. 'entity_type' is also required in this case.
 *      - 'entity_type': (Optional) The entity type of the recipient entity.
 */
function hook_user_message_recipients($m) {
  if ($m->type == 'common') {
    // Admin shall receive all common messages.
    $account = user_load(1);
    $group = array(
      'admin' => array(
        'group label' => 'Administrator',
        'items' => array(),
      ),
    );
    $group['admin']['items'][] = array(
      'label'  => $account->name,
      'emails' => array($account->mail),
      'entity_id' => $account->uid,
      'entity_type' => 'user',
    );
    return $group;
  }
}

/**
 * Add further instructions when a user_message object is being read by an entity.
 * 
 * @param $m
 *  The user_message object.
 * @param $entity
 *  The entity which is reading the user_message.
 * @param $entity_type
 *  The type of the reading entity.
 */
function hook_user_message_read($m, $entity, $entity_type) {
  global $user;
  $account = user_load($user->uid);
  $status = $m->readStatus($account, 'user');
  if ($status) {
    drupal_set_message(t('You have read this message on @date', array('@date' => format_date($status))));
  }
}

/**
 * Add further instructions when a user_message object is being archived.
 * 
 * @param $m
 *  The user_message object.
 * @param $entity
 *  The entity which wants to archive the user_message.
 * @param $entity_type
 *  The type of the entity which wants to archive.
 */
function hook_user_message_archive($m, $entity, $entity_type) {
  // @todo Example.
}

/**
 * Determine the proper received item for the current user account object.
 * 
 * @param $m
 *  The user_message object.
 * @param $account
 *  The user account to get the proper received item for.
 * @param $item
 *  The current object set as proper item,
 *  or FALSE if the user has no proper item yet.
 * 
 * @return
 *  A new item object if you want the user to receive the given user message.
 *  Otherwise, return the already given $item argument.
 */
function hook_user_message_proper_received_item($m, $account, $item) {
  if (!$item) {
    $values = array(
      'groupkey' => 'yourgroup',
      'delta' => 'yourdelta',
      'umid' => $m->umid,
      'label' => 'Example',
      'path' => 'example/1',
      'entity_id' => NULL,
      'entity_type' => NULL,
    );
    // The user_message module will save this new object automatically when required.
    $item = (object) $values;
  }
  return $item;
}

/**
 * Add or change the values for the user_message object which is being created.
 * 
 * @param $m
 *  The user_message object which is being created.
 */
function hook_user_message_create($m) {
  $m->subject = 'Insert subject here';
  
  // Run token_replace() on the message_text field, if defined.
  if (!isset($m->message_text)) {
    $info = field_info_instance('user_message', 'message_text', $m->type);
    if (isset($info) && !empty($info['default_value'])) {
      $data = array(
        'node' => node_load(1),
      );
      foreach ($info['default_value'] as $default) {
        $value = token_replace($default['value'], $data);
        $m->message_text[LANGUAGE_NONE][] = array(
          'value' => $value,
          'format' => $default['format'],
        );
      }
    }
  }
}

/**
 * Add or change the values for the answer which is being created for a user_message.
 * 
 * @param $m
 *  The user_message object to create the answer for.
 * @param $answer
 *  The current answer user_message object.
 */
function hook_user_message_create_answer($m, $answer) {
  $answer->subject = 'Reply to: ' . $m->subject;
}

/**
 * Alter the transmission behavior by acting on the given user_message
 * or by changing the list of recipients this message is being sent to.
 * 
 * The transmission process will be completed after this hook,
 * i.e. changing the recipient data will affect the transmission result.
 * 
 * @param &$data
 *  An array containing the relevant data on transmission with following keys:
 *   - user_message: The user_message object which is being transmitted.
 *   - recipients: The current recipient list this message is being sent to.
 */
function hook_user_message_transmit_alter(&$data) {
  $recipients = &$data['recipients'];
  
  // Admin shall always receive all messages.
  $account = user_load(1);
  $recipients['admin'] = array(
    'group label' => 'Administrator',
    'items' => array(),
  );
  $recipients['admin']['items'][] = array(
    'label'  => $account->name,
    'emails' => array($account->mail),
    'entity_id' => $account->uid,
    'entity_type' => 'user',
  );
}

/**
 * Alter variables and add further preprocess instructions
 * before a user_message is getting themed.
 * 
 * @param &$vars
 *  An array of variables which will be passed to the template / theme function.
 * 
 * @see user_message_preprocess_entity().
 */
function hook_preprocess_user_message_alter(&$vars) {
  $vars['content']['my_field'] = array(
    '#markup' => '<div>Some further content to be displayed...</div>',
  );
}
