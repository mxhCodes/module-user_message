<?php
/**
 * @file
 *  Rules integration for the user_message module. 
 */

/**
 * Implements hook_rules_action_info(). 
 */
function user_message_rules_action_info() {
  $entityinfo = entity_get_info('user_message');
  return array(
    'user_message_transmit' => array(
      'label' => t('Transmit user message'),
      'parameter' => array(
        'user_message' => array('type' => 'user_message', 'label' => t('User message to transmit')),
      ),
      'group' => $entityinfo['label'],
      'base' => 'user_message_transmit_action',
    ),
  );
}
