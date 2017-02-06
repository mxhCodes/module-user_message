<?php

/**
 * @file
 * Theme implementation for user_message entities being rendered in view_mode email.
 *
 *  Available variables:
 * - $user_message: The current user_message object.
 * - $content: An array of field items. Use render($content) to print them all, or
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
<div class="user-message-mail">
  <p class="user-message-created-by">Von <?php print format_username($created_by); ?><?php if ($show_email && !empty($created_by->mail)): ?> (E-Mail: <?php print $created_by->mail; ?>)<?php endif; ?> am <?php print format_date($user_message->stamp); ?></p>

  <p<?php print $title_attributes; ?>>
    Betreff: <?php print $title; ?>
  </p>

  <p class="content">
    <?php
      /**
       * Use paragraphs instead of div blocks.
       * Otherwise, line-breaks won't be added.
       * @see user_message_prepare_mail() and drupal_html_to_text().
       */
      $rendered_content = render($content);
      $rendered_content = str_replace('<div', '<p', $rendered_content);
      $rendered_content = str_replace('</div', '</p', $rendered_content);
      print $rendered_content;
     ?>
  <p>
  
  <p class="user-message-view-online">
    Diese Mitteilung kann online beantwortet werden unter der Adresse<br />
    <?php print $url; ?><br />
  </p>
</div>
