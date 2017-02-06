<?php

/**
 * @file
 *  Page callbacks for the user_message module.
 */

function user_message_admin_overview_page() {
  $query = db_select('user_message', 'user_message')
    ->fields('user_message', array('umid'))
    ->orderBy('user_message.created_date', 'DESC');
  $query = $query->extend('PagerDefault');
  $query->limit(10);
  
  $umids = $query->execute()->fetchCol();
  
  $messages = user_message_load_multiple($umids);
  $content = entity_view('user_message', $messages, 'list_item');
  
  $content['pager'] = array('#theme' => 'pager');
  
  return $content;
}

function user_message_inbox_page() {
  global $user;
  $account = user_load($user->uid);
  
  $messages = user_message_received($account, 10);
  $messages = user_message_accessible_objects('view', $messages);
  $content = entity_view('user_message', $messages, 'list_item');
  
  $content['pager'] = array('#theme' => 'pager');
  
  drupal_set_title('Mitteilungen');
  
  return $messages ? $content : 'Keine Mitteilungen empfangen.';
}

function user_message_outbox_page() {
  global $user;
  $account = user_load($user->uid);
  
  $messages = user_message_transmitted($account, 10);
  $messages = user_message_accessible_objects('view', $messages);
  $content = entity_view('user_message', $messages, 'list_item');
  
  $content['pager'] = array('#theme' => 'pager');
  
  drupal_set_title('Mitteilungen');
  
  return $messages ? $content : 'Sie haben keine Mitteilungen versandt.';
}

function user_message_draft_page() {
  global $user;
  $account = user_load($user->uid);
  
  $messages = user_message_draft($account, 10);
  $messages = user_message_accessible_objects('view', $messages);
  $content = entity_view('user_message', $messages, 'list_item');
  
  $content['pager'] = array('#theme' => 'pager');
  
  drupal_set_title('Mitteilungen');
  
  return $messages ? $content : 'Keine Entwürfe vorhanden.';
}

function user_message_create_page($type = NULL) {
  if (!isset($type) || !$type) {
    // No type argument provided.
    // Use common type as fallback.
    $type = 'common';
  }
  else {
    // Validate type.
    $info = entity_get_info('user_message');
    if (!isset($info['bundles'][$type])) {
      // Provided type argument is not valid.
      // Use common type as fallback.
      $type = 'common';
    }
  }
  
  $m = user_message_create(array('type' => $type));
  $form = drupal_get_form('user_message_default_form', $m);
  
  drupal_set_title('Mitteilungen');
  
  return $form;
}

function user_message_page($m) {
  global $user;
  
  $content = entity_view('user_message', array($m), 'full');
  
  if ($m->status == 'draft') {
    $warning = 'Die Mitteilung wurde noch nicht versendet.';
    if ($m->created_by == $user->uid) {
      $warning .= ' Kontrollieren Sie vor Versandt Ihre Eingaben.';
    }
    drupal_set_message($warning, 'warning', FALSE);
    $content['send_form'] = drupal_get_form('user_message_send_form', $m);
  }
  
  if (($attached = $m->attachedData()) && isset($attached['replied_by_mail'])) {
    $date = format_date($attached['replied_by_mail']['stamp']);
    $username = format_username(user_load($attached['replied_by_mail']['uid']));
    drupal_set_message('Diese Mitteilung wurde am ' . $date . ' von ' . $username . ' per E-Mail beantwortet.');
  }
  
  return $content;
}

function user_message_transmission_finish_page($m) {
  // @todo call $m->finishTransmission(TRUE);
  // -> Batch operation to send all emails.
  return 'Finish transmission page for ' . $m->subject;
}

function user_message_received_page($m) {
  $recipients = $m->recipients();
  
  $items = $m->receivedItems();
  $content = array();
  
  $content['user_message'] = entity_view('user_message', array($m), 'list_item');
  
  foreach ($items as $groupkey => $group) {
    $content[$groupkey] = array(
      '#theme' => 'item_list',
      '#title' => $recipients[$groupkey]['group label'],
      '#attributes' => array(),
    );
    foreach ($group as $delta => $received) {
      $text = UserMessageReceived::recipientLabel($received);
      $uri = UserMessageReceived::recipientUri($received);
      $list_item = isset($uri['path']) ? l($text, $uri['path']) : $text;
      
      // Sent E-Mails.
      if ($mails = UserMessageReceived::sentMails($received)) {
         $list_item .= ' (Gesendet an ' . implode(' | ', $mails) . ')';
      }
      
      // Read status on entities.
      if ($recipient = UserMessageReceived::recipientEntity($received)) {
        $list_item .= ', Gelesen: ';
        $read_status = $m->readStatus($recipient, $received->entity_type);
        $list_item .= $read_status ? 'Am ' . format_date($read_status) : 'Nein';
      }
      
      $content[$groupkey]['#items'][$delta] = $list_item;
    }
  }

  return $content;
}

function user_message_archive_inbox_page() {
  global $user;
  $account = user_load($user->uid);
  
  $messages = user_message_received($account, 10, 0, TRUE);
  $messages = user_message_accessible_objects('view', $messages);
  $content = entity_view('user_message', $messages, 'list_item');
  
  $content['pager'] = array('#theme' => 'pager');
  
  drupal_set_title('Mitteilungen');
  
  return $messages ? $content : 'Sie haben keine empfangenen Mitteilungen archiviert.';
}

function user_message_archive_outbox_page() {
  global $user;
  $account = user_load($user->uid);
  
  $messages = user_message_transmitted($account, 10, 0, TRUE);
  $messages = user_message_accessible_objects('view', $messages);
  $content = entity_view('user_message', $messages, 'list_item');
  
  $content['pager'] = array('#theme' => 'pager');
  
  drupal_set_title('Mitteilungen');
  
  return $messages ? $content : 'Sie haben keine versendeten Mitteilungen archiviert.';
}

function user_message_config_page() {
  $content = array();
  
  $content['description'] = array(
    '#type' => 'container',
    '#attributes' => array(),
    '#children' => 'Klicken Sie auf eine der verfügbaren Mitteilungstypen, um diese zu konfigurieren.',
  );
  
  $info = entity_get_info('user_message');
  $items = array();
  foreach ($info['bundles'] as $type => $value) {
    $items[] = l($value['label'], 'admin/config/user-message/' . $type);
  }
  $content['types'] = array(
    '#theme' => 'item_list',
    '#items' => $items,
  );
  
  return $content;
}

function user_message_type_config_page($type) {
  return 'Maschinenlesbarer Name: ' . $type;
}

function user_message_answer_page($m) {
  
  $answer = user_message_create_answer($m);
  
  $content = array();
  $content['user_message'] = entity_view('user_message', array($m), 'compact');
  $content['answer_form'] = drupal_get_form('user_message_answer_form', $answer);
  
  return $content;
}

function user_message_answer_form($form, &$form_state, $answer) {
  $form = user_message_default_form($form, $form_state, $answer);
  $form['actions']['submit']['#value'] = 'Antwort verfassen';
  
  // All recipient fields shall be embedded in a collapsed fieldset.
  $form['recipient'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#title' => 'Empfänger',
    '#weight' => 20,
  );
  $info = field_info_instances('user_message', $answer->type);
  foreach ($info as $info_instance) {
    $field = $info_instance['field_name'];
    if (strpos($field, 'recipient') !== FALSE) {
      $form['recipient'][$field] = $form[$field];
      unset($form[$field]);
    }
  }
  
  return $form;
}

function user_message_send_form($form, &$form_state, $m) {
  $form = user_message_default_form($form, $form_state, $m);
  $form['actions']['submit']['#value'] = 'Neu verfassen';
  
  $form['confirm_actions'] = array(
    '#weight' => 10,
  );
  $form['confirm_actions']['confirm'] = array(
    '#type' => 'submit',
    '#name' => 'confirm',
    '#value' => 'Bestätigen und Versenden',
    '#weight' => 10,
    '#submit' => array('user_message_send_form_confirm'),
  );
  $form['confirm_actions']['draft'] = array(
    '#type' => 'submit',
    '#name' => 'draft',
    '#value' => 'Als Entwurf speichern',
    '#weight' => 20,
    '#submit' => array('user_message_send_form_draft'),
  );
  $form['confirm_actions']['cancel'] = array(
    '#type' => 'submit',
    '#name' => 'cancel',
    '#value' => 'Verwerfen',
    '#weight' => 30,
    '#submit' => array('user_message_send_form_cancel'),
  );
  
  // Let's embed the edit form into a fieldset.
  $form['edit'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#title' => 'Überarbeiten',
    '#weight' => 20,
  );
  $info = field_info_instances('user_message', $m->type);
  foreach ($info as $info_instance) {
    $field = $info_instance['field_name'];
    $form['edit'][$field] = $form[$field];
    unset($form[$field]);
  }
  $form['edit']['subject'] = $form['subject'];
  $form['edit']['mail_status'] = $form['mail_status'];
  $form['edit']['actions'] = $form['actions'];
  unset($form['subject'], $form['mail_status'], $form['actions']);
  
  return $form;
}

function user_message_send_form_confirm($form, &$form_state) {
  user_message_form_submit_build_entity($form, $form_state);
  $m = $form_state['user_message'];
  
  // A batch operation is required when sending a massive number of emails.
  actions_do('user_message_transmit_action', $m, array('batch' => TRUE));
  
  // Unset the message which is set in user_message_page().
  drupal_get_messages('warning', TRUE);
  
  drupal_set_message('Ihre Mitteilung wurde versandt.');

  $form_state['redirect'] = 'user-message/outbox';
}

function user_message_send_form_draft($form, &$form_state) {
  user_message_form_submit_build_entity($form, $form_state);
  $m = $form_state['user_message'];
  $m->status = 'draft';
  $m->save();
  drupal_set_message('Ihre Mitteilung wurde als Entwurf gespeichert.');
  $form_state['redirect'] = 'user-message/draft';
}

function user_message_send_form_cancel($form, &$form_state) {
  $m = $form_state['user_message'];
  $m->delete();
  
  // Unset the message which is set in user_message_page().
  drupal_get_messages('warning', TRUE);
  
  drupal_set_message('Ihre Mitteilung wurde nicht gesendet und stattdessen verworfen.');
  $form_state['redirect'] = 'user-message/outbox';
}

function user_message_default_form($form, &$form_state, $m = NULL) {
  $form_state['entity_type'] = 'user_message';
  $form_state['user_message'] = $m;
  
  $form['subject'] = array(
    '#type' => 'textfield',
    '#title'=> 'Betreff',
    '#default_value' => $m->subject,
    '#required' => TRUE,
    '#size' => 80,
    '#weight' => 100,
  );
  $form['mail_status'] = array(
    '#type' => 'checkbox',
    '#title' => 'Alle Empfänger auch per E-Mail benachrichtigen',
    '#default_value' => $m->mail_status,
    '#return_value' => 'send_mail',
    '#weight' => 9900,
  );
  $form['actions'] = array(
    '#weight' => 10000,
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#name' => 'submit',
    '#value' => 'Verfassen',
    '#weight' => 10,
    '#submit' => array('user_message_default_form_submit'),
  );
  $form['#validate'] = array('user_message_default_form_validate');
  field_attach_form('user_message', $m, $form, $form_state);

  return $form;
}

function user_message_default_form_submit($form, &$form_state) {
  user_message_form_submit_build_entity($form, $form_state);
  $m = $form_state['user_message'];
  $m->save();
  $form_state['redirect'] = 'user-message/' . $m->umid;
}

function user_message_default_form_validate($form, &$form_state) {
  $m = $form_state['user_message'];
  $form_state['values']['type'] = $m->type;
  entity_form_field_validate('user_message', $form, $form_state);
  // @todo form_set_error('actions', 'Es wurde kein Empfänger erkannt.');
}

function user_message_form_submit_build_entity($form, &$form_state) {
  $m = $form_state['user_message'];
  entity_form_submit_build_entity('user_message', $m, $form, $form_state);
  return $m;
}
