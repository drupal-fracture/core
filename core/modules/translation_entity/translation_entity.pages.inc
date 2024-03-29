<?php

/**
 * @file
 * The entity translation user interface.
 */

use Drupal\Core\Language\Language;
use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Entity\EntityNG;

/**
 * Translations overview page callback.
 *
 * @param \Drupal\Core\Entity\EntityInterface $entity
 *   The entity whose translation overview should be displayed.
 */
function translation_entity_overview(EntityInterface $entity) {
  $controller = translation_entity_controller($entity->entityType());
  $languages = language_list();
  $original = $entity->language()->langcode;
  $translations = $entity->getTranslationLanguages();
  $field_ui = module_exists('field_ui') && user_access('administer ' . $entity->entityType() . ' fields');

  $path = $controller->getViewPath($entity);
  $base_path = $controller->getBasePath($entity);
  $edit_path = $controller->getEditPath($entity);

  $header = array(t('Language'), t('Translation'), t('Source language'), t('Status'), t('Operations'));
  $rows = array();

  if (language_multilingual()) {
    // If we have a view path defined for the current entity get the switch
    // links based on it.
    if ($path) {
      $links = _translation_entity_get_switch_links($path);
    }

    // Determine whether the current entity is translatable.
    $translatable = FALSE;
    foreach (field_info_instances($entity->entityType(), $entity->bundle()) as $instance) {
      $field_name = $instance['field_name'];
      $field = field_info_field($field_name);
      if ($field['translatable']) {
        $translatable = TRUE;
        break;
      }
    }

    foreach ($languages as $language) {
      $language_name = $language->name;
      $langcode = $language->langcode;
      $add_path = $base_path . '/translations/add/' . $original . '/' . $langcode;
      $translate_path = $base_path . '/translations/edit/' . $langcode;
      $delete_path = $base_path . '/translations/delete/' . $langcode;

      if ($base_path) {
        $add_links = _translation_entity_get_switch_links($add_path);
        $edit_links = _translation_entity_get_switch_links($edit_path);
        $translate_links = _translation_entity_get_switch_links($translate_path);
        $delete_links = _translation_entity_get_switch_links($delete_path);
      }

      $operations = array(
        'data' => array(
          '#type' => 'operations',
          '#links' => array(),
        ),
      );
      $links = &$operations['data']['#links'];

      if (isset($translations[$langcode])) {
        // Existing translation in the translation set: display status.
        $source = isset($entity->source[$langcode]) ? $entity->source[$langcode] : '';
        $is_original = $langcode == $original;
        $translation = $translations[$langcode];
        $label = $entity->label($langcode);
        $link = isset($links->links[$langcode]['href']) ? $links->links[$langcode] : array('href' => $path, 'language' => $language);
        $row_title = l($label, $link['href'], $link);

        if (empty($link['href'])) {
          $row_title = $is_original ? $label : t('n/a');
        }

        // If the user is allowed to edit the entity we point the edit link to
        // the entity form, otherwise if we are not dealing with the original
        // language we point the link to the translation form.
        if ($edit_path && $controller->getAccess($entity, 'update')) {
          $links['edit'] = isset($edit_links->links[$langcode]['href']) ? $edit_links->links[$langcode] : array('href' => $edit_path, 'language' => $language);
        }
        elseif (!$is_original && $controller->getTranslationAccess($entity, 'update')) {
          $links['edit'] = isset($translate_links->links[$langcode]['href']) ? $translate_links->links[$langcode] : array('href' => $translate_path, 'language' => $language);
        }

        if (isset($links['edit'])) {
          $links['edit']['title'] = t('edit');
        }

        // @todo Consider supporting the ability to track translation publishing
        // status independently from entity status, as it may not exist.
        $translation = $entity->getTranslation($langcode, FALSE);
        $status = !isset($translation->status) || $translation->status ? t('Published') : t('Not published');
        // @todo Add a theming function here.
        $status = '<span class="status">' . $status . '</span>' . (!empty($entity->retranslate[$langcode]) ? ' <span class="marker">' . t('outdated') . '</span>' : '');

        if ($is_original) {
          $language_name = t('<strong>@language_name</strong>', array('@language_name' => $language_name));
          $source_name = t('n/a');
        }
        else {
          $source_name = isset($languages[$source]) ? $languages[$source]->name : t('n/a');
          if ($controller->getTranslationAccess($entity, 'delete')) {
            $links['delete'] = isset($delete_links->links[$langcode]['href']) ? $delete_links->links[$langcode] : array('href' => $delete_links, 'language' => $language);
            $links['delete']['title'] = t('delete');
          }
        }
      }
      else {
        // No such translation in the set yet: help user to create it.
        $row_title = $source_name = t('n/a');
        $source = $entity->language()->langcode;

        if ($source != $langcode && $controller->getTranslationAccess($entity, 'create')) {
          if ($translatable) {
            $links['add'] = isset($add_links->links[$langcode]['href']) ? $add_links->links[$langcode] : array('href' => $add_path, 'language' => $language);
            $links['add']['title'] = t('add');
          }
          elseif ($field_ui) {
            $entity_path = field_ui_bundle_admin_path($entity->entityType(), $entity->bundle());
            // Link directly to the fields tab to make it easier to find the
            // setting to enable translation on fields.
            $path = $entity_path . '/fields';
            $links['nofields'] = array('title' => t('no translatable fields'), 'href' => $path, 'language' => $language);
          }
        }

        $status = t('Not translated');
      }

      $rows[] = array($language_name, $row_title, $source_name, $status, $operations);
    }
  }

  drupal_set_title(t('Translations of %label', array('%label' => $entity->label())), PASS_THROUGH);

  // Add metadata to the build render array to let other modules know about
  // which entity this is.
  $build['#entity'] = $entity;

  $build['translation_entity_overview'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
  );

  return $build;
}

/**
 * Returns the localized links for the given path.
 *
 * @param string $path
 *   The path for which language switch links should be provided.
 *
 * @returns
 *   A renderable array of language switch links.
 */
function _translation_entity_get_switch_links($path) {
  $links = language_negotiation_get_switch_links(LANGUAGE_TYPE_CONTENT, $path);
  if (empty($links)) {
    // If content language is set up to fall back to the interface language,
    // then there will be no switch links for LANGUAGE_TYPE_CONTENT, ergo we
    // also need to use interface switch links.
    $links = language_negotiation_get_switch_links(LANGUAGE_TYPE_INTERFACE, $path);
  }
  return $links;
}

/**
 * Page callback for the translation addition page.
 *
 * @param EntityInterface $entity
 *   The entity being translated.
 * @param Language $source
 *   (optional) The language of the values being translated. Defaults to the
 *   entity language.
 * @param Language $target
 *   (optional) The language of the translated values. Defaults to the current
 *   content language.
 *
 * @return array
 *   A processed form array ready to be rendered.
 */
function translation_entity_add_page(EntityInterface $entity, Language $source = NULL, Language $target = NULL) {
  $source = !empty($source) ? $source : $entity->language();
  $target = !empty($target) ? $target : language(LANGUAGE_TYPE_CONTENT);
  // @todo Exploit the upcoming hook_entity_prepare() when available.
  translation_entity_prepare_translation($entity, $source, $target);
  $info = $entity->entityInfo();
  $operation = isset($info['default_operation']) ? $info['default_operation'] : 'default';
  $form_state = entity_form_state_defaults($entity, $operation, $target->langcode);
  $form_state['translation_entity']['source'] = $source;
  $form_state['translation_entity']['target'] = $target;
  $controller = translation_entity_controller($entity->entityType());
  $form_state['translation_entity']['translation_form'] = !$controller->getAccess($entity, 'update');
  $form_id = entity_form_id($entity);
  return drupal_build_form($form_id, $form_state);
}

/**
 * Page callback for the translation edit page.
 *
 * @param \Drupal\Core\Entity\EntityInterface $entity
 *   The entity being translated.
 * @param \Drupal\Core\Language\Language $language
 *   (optional) The language of the translated values. Defaults to the current
 *   content language.
 *
 * @return array
 *   A processed form array ready to be rendered.
 */
function translation_entity_edit_page(EntityInterface $entity, Language $language = NULL) {
  $language = !empty($language) ? $language : language(LANGUAGE_TYPE_CONTENT);
  $info = $entity->entityInfo();
  $operation = isset($info['default_operation']) ? $info['default_operation'] : 'default';
  $form_state = entity_form_state_defaults($entity, $operation, $language->langcode);
  $form_state['translation_entity']['translation_form'] = TRUE;
  $form_id = entity_form_id($entity);
  return drupal_build_form($form_id, $form_state);
}

/**
 * Populates target values with the source values.
 *
 * @param \Drupal\Core\Entity\EntityInterface $entity
 *   The entitiy being translated.
 * @param \Drupal\Core\Language\Language $source
 *   The language to be used as source.
 * @param \Drupal\Core\Language\Language $target
 *   The language to be used as target.
 */
function translation_entity_prepare_translation(EntityInterface $entity, Language $source, Language $target) {
  // @todo Unify field and property handling.
  $instances = field_info_instances($entity->entityType(), $entity->bundle());
  if ($entity instanceof EntityNG) {
    $source_translation = $entity->getTranslation($source->langcode);
    $target_translation = $entity->getTranslation($target->langcode);
    foreach ($target_translation->getPropertyDefinitions() as $property_name => $definition) {
      // @todo The value part should not be needed. Remove it as soon as things
      //   do not break.
      $target_translation->$property_name->value = $source_translation->$property_name->value;
    }
  }
  else {
    foreach ($instances as $field_name => $instance) {
      $field = field_info_field($field_name);
      if (!empty($field['translatable'])) {
        $value = $entity->get($field_name);
        $value[$target->langcode] = isset($value[$source->langcode]) ? $value[$source->langcode] : array();
        $entity->set($field_name, $value);
      }
    }
  }
}

/**
 * Form constructor for the translation deletion confirmation.
 */
function translation_entity_delete_confirm(array $form, array $form_state, EntityInterface $entity, Language $language) {
  $langcode = $language->langcode;
  $controller = translation_entity_controller($entity->entityType());

  return confirm_form(
    $form,
    t('Are you sure you want to delete the @language translation of %label?', array('@language' => $language->name, '%label' => $entity->label())),
    $controller->getEditPath($entity),
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Form submission handler for translation_entity_delete_confirm().
 */
function translation_entity_delete_confirm_submit(array $form, array &$form_state) {
  list($entity, $language) = $form_state['build_info']['args'];
  $controller = translation_entity_controller($entity->entityType());

  // Remove the translated values.
  $controller->removeTranslation($entity, $language->langcode);
  $entity->save();

  // Remove any existing path alias for the removed translation.
  if (module_exists('path')) {
    $conditions = array('source' => $controller->getViewPath($entity), 'langcode' => $language->langcode);
    drupal_container()->get('path.crud')->delete($conditions);
  }

  $form_state['redirect'] = $controller->getBasePath($entity) . '/translations';
}
