<?php

$plugin = array(
  'single' => TRUE,
  'title' => t('Programs: Content Panes'),
  'description' => t('Shows a program and program instance fields with basic configuration options.'),
  'category' => t('Custom Content & Features'),
  'edit form' => 'gsb_feature_program_display_edit_form',
  'render callback' => 'gsb_feature_program_display_render',
  'admin info' => 'gsb_feature_program_display_admin_info',
  'admin title' => 'gsb_feature_program_display_admin_title',
  'defaults' => array('viewmodes' => '')
);

/**
 * The edit form the "Programs: Content Panes" Content Panes
 */
function gsb_feature_program_display_edit_form($form, &$form_state) {

  $conf = $form_state['conf'];

  $view_modes = array();

  // Get the list of program view modes
  $view_mode_settings = field_view_mode_settings('node', 'program');

  $entity_info = entity_get_info('node');

  foreach ($view_mode_settings as $view_mode => $setting) {
    if ($setting['custom_settings']) {
      if (strpos($view_mode, 'field_') === 0) {
        $view_modes['program_' . $view_mode] = $entity_info['view modes'][$view_mode]['label'];
      }
    }
  }

  // Get the list of program instance view modes
  $view_mode_settings = field_view_mode_settings('node', 'program_instance');
  foreach ($view_mode_settings as $view_mode => $setting) {
    if ($setting['custom_settings']) {
      if (strpos($view_mode, 'field_') === 0) {
        $view_modes['program_instance_' . $view_mode] = $entity_info['view modes'][$view_mode]['label'];
      }
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
    $program = _gsb_feature_program_get_cached_node($program_nodes, $program_nid);
  }
  else {
    $program = entity_load('node', array($program_nid));
    $program_nodes[$program_nid] = $program;
    cache_set('gsb_feature_program_display_data', $program_nodes, 'cache');
  }

  // valid view modes use the following naming convention:
  //   program nodes use view modes starting with: 'program_'
  //   program_instance nodes use view modes starting with: 'program_instance_'

  if (strpos($view_mode, 'program_instance_') === 0) {
    // Output content for the program instance

    // Lookup the program instances for this program
    $query = db_select('node', 'n');
    $query->fields('n', array('nid', 'status', 'type'));
    $query->join('field_data_field_program', 'fp', "n.nid = fp.entity_id AND fp.entity_type = 'node' AND fp.deleted = '0'");
    $query->fields('fp', array('entity_id', 'entity_type', 'deleted', 'field_program_target_id'));
    $query->join('node', 'nodefp', 'fp.field_program_target_id = nodefp.nid');
    $query->fields('nodefp', array('nid'));
    $query->join('field_data_field_instance_date', 'fid', "n.nid = fid.entity_id AND fid.entity_type = 'node' AND fid.deleted = '0'");
    $query->fields('fid', array('field_instance_date_value'));
    $query->join('field_data_field_display_on_finder', 'dof', "n.nid = dof.entity_id AND dof.entity_type = 'node' AND dof.deleted = '0'");
    $query->fields('dof', array('field_display_on_finder_value'));
    $query->join('field_data_field_is_application_open', 'ao', "n.nid = ao.entity_id AND ao.entity_type = 'node' AND ao.deleted = '0'");
    $query->fields('ao', array('field_is_application_open_value'));
    $query->condition('nodefp.nid', $program_nid);
    $query->condition('n.status', '1');
    $query->condition('n.type', 'program_instance');
    $query->condition('dof.field_display_on_finder_value', '1');
    $query->condition('ao.field_is_application_open_value', '1');
    $query->orderBy('fid.field_instance_date_value', 'ASC');
    $query->range(0,1);
    $results = $query->execute();

    foreach ($results as $record) {
      if (isset($program_nodes[$record->nid])) {
        $program_instance = _gsb_feature_program_get_cached_node($program_nodes, $record->nid);
      }
      else {
        $program_instance = entity_load('node', array($record->nid));
        $program_nodes[$record->nid] = $program_instance;
        cache_set('gsb_feature_program_display_data', $program_nodes, 'cache');
      }
      // Render the program instance using the view mode
      $entity_array = entity_view('node', $program_instance, preg_replace('/program_instance_/','',$view_mode, 1));
      $content = drupal_render($entity_array);
      // Add in the content
      $block->content .= $content;
    }
  }
  else if (strpos($view_mode, 'program_') === 0) {
    // Output content for the program
    // Render the program instance using the view mode
    //$view_mode = 'default';
    $entity_array = entity_view('node', $program, preg_replace('/program_/','',$view_mode, 1));
    $content = drupal_render($entity_array);
    // Add in the content
    $block->content .= $content;
  }

  return $block;
}

function _gsb_feature_program_get_cached_node(&$program_nodes, $nid) {
  $node = $program_nodes[$nid];
  $query = db_select('node_revision', 'nr');
  $query->fields('nr', array('nid', 'status', 'timestamp'));
  $query->condition('nr.nid', $nid);
  $query->condition('nr.status', '1');
  $results = $query->execute();
  foreach ($results as $record) {
    if ($node[$nid]->revision_timestamp != $record->timestamp) {
      $node = entity_load('node', array($nid));
      $program_nodes[$nid] = $node;
      cache_set('gsb_feature_program_display_data', $program_nodes, 'cache');
    }
  }
  return $node;
}

/**
 * If the pane is a program or program_instance viewmode-based pane,
 * then change the title for the pane to be the viewmode's label value.
 */
function gsb_feature_program_display_admin_title($subtype, $conf, $context = NULL) {
  if (!empty($conf['viewmodes'])) {
    $needle = 'program_instance_field_';
    $pos = strpos($conf['viewmodes'], $needle);
    if ($pos === 0) {
      $needle = 'program_instance_';
      $entity_info = entity_get_info('node');
      $viewmode = substr($conf['viewmodes'], strlen($needle));
      $output = t('pi: ' . $entity_info['view modes'][$viewmode]['label']);
      return $output;
    }
    $needle = 'program_field_';
    $pos = strpos($conf['viewmodes'], $needle);
    if ($pos === 0) {
      $needle = 'program_';
      $entity_info = entity_get_info('node');
      $viewmode = substr($conf['viewmodes'], strlen($needle));
      $output = t('p: ' . $entity_info['view modes'][$viewmode]['label']);
      return $output;
    }
  }
  // otherwise default by returning the plugin's title
  $plugin = ctools_get_content_type($subtype);
  return $plugin['title'];
}
