<?php

$plugin = array(
  'single' => TRUE,
  'title' => t('Program/Program Instance Fields Display'),
  'description' => t('Shows a program and program instance fields with basic configuration options.'),
  'category' => t('Program/Program Instance'),
  'edit form' => 'gsb_feature_program_display_edit_form',
  'render callback' => 'gsb_feature_program_display_render',
  'admin info' => 'gsb_feature_program_display_admin_info',
  'defaults' => array('viewmodes' => '')
);

 /**
  * The edit form the "Program/Program Instance Fields Display" Content Panes
  */
function gsb_feature_program_display_edit_form($form, &$form_state) {

  $conf = $form_state['conf'];

  $view_modes = array();

  // Get the list of program view modes
  $view_mode_settings = field_view_mode_settings('node', 'program');
  foreach ($view_mode_settings as $view_mode => $setting) {
    if ($setting['custom_settings']) {
      $view_modes[$view_mode] = $view_mode;
    }
  }

  // Get the list of program instance view modes
  $view_mode_settings = field_view_mode_settings('node', 'program_instance');
  foreach ($view_mode_settings as $view_mode => $setting) {
    if ($setting['custom_settings']) {
      $view_modes[$view_mode] = $view_mode;
    }
  }

  $form['viewmodes'] = array(
    '#prefix' => '',
     '#type' => 'select',
     '#title' => t('View Modes'),
     '#description' => t('The list of view modes defined for program.'),
     '#default_value' => $conf['viewmodes'],
     '#options' => $view_modes,
     '#required' => TRUE,
  );

  return $form;
}

 /**
  * The submit form stores the data in $conf.
  */
function gsb_feature_program_display_edit_form_submit($form, &$form_state) {
  foreach (array_keys($form_state['plugin']['defaults']) as $key) {
    if (isset($form_state['values'][$key])) {
      $form_state['conf'][$key] = $form_state['values'][$key];
    }
  }
}

/**
 * Run-time rendering of the body of the block (content type)
 * See ctools_plugin_examples for more advanced info
 */
function gsb_feature_program_display_render($subtype, $conf, $panel_args, $context = NULL) {
  $program_nodes = &drupal_static(__FUNCTION__);

  // Get the saved view mode
  $view_mode = empty($conf['viewmodes']) ? 'default' : $conf['viewmodes'];

  // Create the content block
  $block = new stdClass();

  // Initial content is blank
  $block->title = '';
  $block->content = '';

  // Get the node for the current Program Panelized Page
  $ppp = menu_get_object();

  // Get the program nid for the PPP
  $program_nid = $ppp->field_program['und'][0]['target_id'];

  // See if we have any of the program and program instance nodes already
  // loaded in our cache.
  if (!isset($program_nodes)) {
    if ($cache = cache_get('gsb_feature_program_display_data')) {
      $program_nodes = $cache->data;
    }
    else {
      $program_nodes = array();
      $program = entity_load('node', array($program_nid));
      $program_nodes[$program_nid] = $program;
      cache_set('gsb_feature_program_display_data', $program_nodes, 'cache');
    }
  }

  if (isset($program_nodes[$program_nid])) {
    $program = $program_nodes[$program_nid];
  }
  else {
    $program = entity_load('node', array($program_nid));
    $program_nodes[$program_nid] = $program;
    cache_set('gsb_feature_program_display_data', $program_nodes, 'cache');
  }

  // valid view modes use the following naming convention:
  //   program nodes use view modes starting with: 'program_'
  //   program_instance nodes use view modes starting with: 'pi_'

  if (strpos($view_mode, 'program_') === 0) {
    // Output content for the program
    // Render the program instance using the view mode
    $entity_array = entity_view('node', $program, $view_mode);
    $content = drupal_render($entity_array);
    // Add in the content
    $block->content .= $content;
  }

  if (strpos($view_mode, 'pi_') === 0) {
    // Output content for the program instance

    // Lookup the program instances for this program
    $query = db_select('node', 'n');
    $query->fields('n', array('nid', 'status', 'type'));
    $query->join('field_data_field_program', 'fp', "n.nid = fp.entity_id AND fp.entity_type = 'node' AND fp.deleted = '0'");
    $query->fields('fp', array('entity_id', 'entity_type', 'deleted', 'field_program_target_id'));
    $query->join('node', 'nodefp', 'fp.field_program_target_id = nodefp.nid');
    $query->fields('nodefp', array('nid'));
    $query->condition('nodefp.nid', $program_nid);
    $query->condition('n.status', '1');
    $query->condition('n.type', 'program_instance');
    $results = $query->execute();

    foreach ($results as $record) {
      if (isset($program_nodes[$record->nid])) {
        $program_instance = $program_nodes[$record->nid];
      }
      else {
        $program_instance = entity_load('node', array($record->nid));
        $program_nodes[$record->nid] = $program_instance;
        cache_set('gsb_feature_program_display_data', $program_nodes, 'cache');
      }
      // Render the program instance using the view mode
      $entity_array = entity_view('node', $program_instance, $view_mode);
      $content = drupal_render($entity_array);
      // Add in the content
      $block->content .= $content;
    }
  }

  return $block;
}
