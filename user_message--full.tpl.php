<?php

/**
 * @file
 * Theme implementation for user_message entities being rendered in view_mode full.
 *
 *  Available variables:
 * - $user_message: The current user_message object.
 * - $content: An array of comment items. Use render($content) to print them all, or
 *   print a subset such as render($content['field_example']). Use
 *   hide($content['field_example']) to temporarily suppress the printing of a
 *   given element.
 * - $title: The (sanitized) entity label.
 * - $url: Direct url of the current entity if specified.
 * - $page: Flag for the full page state.
 * - $classes: String of classes that can be used to style contextually through
 *   CSS. It can be manipulated through the variable $classes_array from
 *   preprocess functions. By default the following classes are available, where
 *   the parts enclosed by {} are replaced by the appropriate values:
 *   - entity-{ENTITY_TYPE}
 *   - {ENTITY_TYPE}-{BUNDLE}
 *
 * Other variables:
 * - $classes_array: Array of html class attribute values. It is flattened
 *   into a string within the variable $classes.
 *
 * @see template_preprocess()
 * @see template_preprocess_entity()
 * @see template_process()
 */
?>
<div id="user-message-<?php print $user_message->umid; ?>" class="<?php print $classes; ?> clearfix"<?php print $attributes; ?>>

  <div class="user-message-created-by">
    <span>
      Von <?php print format_username($created_by); ?>
      <?php if ($show_email && $created_by->mail): ?>
        (E-Mail: <a href="mailto:<?php print $created_by->mail; ?>"><?php print $created_by->mail; ?></a>)
      <?php endif; ?>
      am <?php print format_date($user_message->stamp); ?>
    </span>
  </div>
  
  <?php if ($recipients): ?>
    <div class="user-message-recipients-info">
      <h4>Empfänger
        <?php if ($view_received): ?>
          [<a href="/user-message/<?php print $user_message->umid; ?>/received">Liste</a>]
        <?php endif; ?>
      </h4>
      <div class="user-message-recipients">
        <?php print $recipients; ?>
      </div>
      <?php if ($mail): ?>
        <div class="user-message-mail-info">
          <?php if ($user_message->mail_status == 'mail_sent'): ?>
            <em>Diese Mitteilung wurde an die Adressaten auch per Mail gesendet.</em>
          <?php else: ?>
            <strong>Diese Mitteilung wird an die Adressaten auch per Mail gesendet.</strong>
          <?php endif; ?>
        </div>
      <?php endif; ?>
    </div>
  <?php endif; ?>

  <h2<?php print $title_attributes; ?>>
    Betreff: <?php print $title; ?>
  </h2>
  
  <?php if ($replied_message): ?>
    <div class="user-message-reply-to">
      <em>
        Dies ist eine Anwort auf 
        <a href="/user-message/<?php print $replied_message->umid; ?>">
          <?php print $replied_message->subject; ?>
        </a>
        von <?php print format_username(user_load($replied_message->created_by)); ?>
      </em>
    </div>
  <?php endif; ?>
  
  <?php if ($user_message != $root_message && $replied_message != $root_message): ?>
    <div class="user-message-goto-root">
      <em>
        Ursprungsnachricht: 
        <a href="/user-message/<?php print $root_message->umid; ?>">
          <?php print $root_message->subject; ?>
        </a>
        von <?php print format_username(user_load($root_message->created_by)); ?>
      </em>
    </div>
  <?php endif; ?>

  <div class="content"<?php print $content_attributes; ?>>
    <?php
      print render($content);
    ?>
  </div>

  <?php if (!empty($actions)): ?>
    <?php print render($actions); ?>
  <?php endif; ?>

  <?php if ($replies): ?>
    <h2>Antworten</h2>
    <div class="user-message-replies">
      <?php print render($replies); ?>
    </div>
  <?php endif; ?>
</div>
