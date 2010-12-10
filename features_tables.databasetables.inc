<?php
// $Id$


/**
 * @file
 * The databasetables component management file for features_tables.module
 *
 * Manages the export, reverting and rebuilding of database tables as features components
 */


/**
 * List all tables that may be exported.
 *
 * @return array
 *   A keyed array of items, suitable for use with a FormAPI select or
 *   checkboxes element.
 */
function databasetables_features_export_options() {
  $options = array();
  foreach (databasetables_list_tables() as $tables) {
    $options[$tables['name']] = $tables['title'];
  }
  return $options;
}


/**
 * Process the export array
 *
 * @param array $data
 *   An array of machine names for the component in question to be exported.
 * @param array &$export
 *   By reference. An array of all components to be exported with a given
 *   feature. Component objects that should be exported should be added to
 *   this array.
 * @param string $module_name
 *   The name of the feature module to be generated.
 * @return array
 *   The pipe array of further processors that should be called.
 */
function databasetables_features_export($data, &$export, $module_name) {
  $export['dependencies']['databasetables'] = 'features_tables';
  $tables = databasetables_list_tables();

  foreach ($data as $key => $component) {
    $export['dependencies'][$tables[$key]['module']] = $tables[$key]['module'];
    $export['features']['databasetables'][$component] = $component;
  }
                                            
  return array();
}


/**
 * Render one or more tables to code.
 *
 * @param string $module_name
 *   The name of the feature module to be exported.
 * @param array $data
 *   An array of machine name identifiers for the objects to be rendered.
 * @return array
 *   An associative array of rendered PHP code where the key is the name of the
 *   hook that should wrap the PHP code. The hook should not include the name
 *   of the module, e.g. the key for `features_tables_example` should simply be `example`.
 */
function databasetables_features_export_render($module_name, $data) {

  module_invoke_all('features_tables_export_preprocess', $module_name, $data);

  $code = array();
  $code[] = '$databasetables = array();';
  ksort($data);
  foreach ($data as $name) {
    $code[] = "\$databasetables['{$name}'] = " . features_var_export(databasetables_export_table($name)) .";";
  }
  $code[] = "return \$databasetables;";
  $code = implode("\n", $code);

  module_invoke_all('features_tables_export_postprocess', $module_name, $data);

  return array('databasetables_defaults' => $code);
}


/**
 *
 * Revert all component objects for a given feature module.
 *
 * @param string $module_name
 *   The name of the feature module whose components should be reverted.
 * @return boolean
 *   TRUE or FALSE for whether the components were successfully reverted.
 */
function databasetables_features_revert($module_name) {
  return databasetables_features_rebuild($module_name);
}


/**
 *
 * Rebuild all component objects for a given feature module. Should only be
 * implemented for 'faux-exportable' components.
 *
 * This hook is called at points where Features determines that it is safe
 * (ie. the feature is in state `FEATURES_REBUILDABLE`) for your module to
 * replace objects in the database with defaults that you collect from your
 * own defaults hook. See API.txt for how Features determines whether a
 * rebuild of components is possible.
 *
 * @param string $module_name
 *   The name of the feature module whose components should be rebuilt.
 */
function databasetables_features_rebuild($module_name) {
  $tables = features_get_default('databasetables', $module_name);
  
  if (!empty($tables)) {               
    $table_names = array_keys($tables);
    module_invoke_all('features_tables_import_preprocess', $module_name, $table_names);
    foreach ($tables as $name => $table) {
      databasetables_import_table($name, $table, $module_name);
    }
    module_invoke_all('features_tables_import_postprocess', $module_name, $table_names);
  }
                                            
}


/**
 * List all tables through schema api
 *
 * @return array
 *   An associative array with table-schema-arrays
 */
function databasetables_list_tables() {
  foreach (drupal_get_schema() as $tablename => $table) {
    $table['title'] = truncate_utf8($table['description'], 50, TRUE, TRUE);
    $table['title'] = $table['title'] ? $table['name'] .' - '. $table['title'] : $table['name'];
    $tables[$table['name']] = $table;
  }

  ksort($tables);

  return $tables;
}


/**
 * Export all row data for a given table name
 *
 * @param string $table_name
 *   The name of the table to retrieve all row data from
 * @return array
 *   A keyed array with hierarchical 'schema' array and the numeric array of the table 'data' row objects
 */
function databasetables_export_table($table_name) {
  $schema = databasetables_reduce_tableschema(drupal_get_schema($table_name));

  $data  = array();
  $sorts = array();
  
  // prepare ORDER BY primary key statement
  if (count($schema['primary key'])) {
    foreach ($schema['primary key'] as $field) {
      $sorts[] = $field . " ASC";
    }
    $sorts = "ORDER BY " . implode(', ', $sorts);
  }

  $query = "SELECT * FROM {$table_name} $sorts";
  $result = db_query($query);

  while ($row = db_fetch_array($result)) {
    $row['!table'] = $table_name;
    drupal_alter('features_tables_export_row', $row);
    unset($row['!table']);
    $data[] = $row;
  }

  return array('schema' => $schema, 'data' => $data);
}


/**
 * Import all stored rows to a table
 *
 * @param string $table_name
 *   The name of the table to import row data to
 * @param array &$table
 *   The structured array describing 'schema' and 'data' of the table (see databasetables_export_table)
 * @param string $module_name
 *   The name of the feature module whose components should be rebuilt.
 */
function databasetables_import_table($table_name, &$table, $module_name) {
  // if schema matches...
  if (serialize(databasetables_reduce_tableschema($table['schema'])) == serialize(databasetables_reduce_tableschema(drupal_get_schema($table_name)))) {
    // schema is compatible:
    // try to truncate table...
    db_query("TRUNCATE {$table_name}");
    $error = db_error();
    // error: truncate failed:
    if ($error) {
      $message = t("Could not rebuild table '%table_name' from feature '%module_name': TRUNCATE of table '%table_name' failed with the following error:\n\n $error", array('%table_name' => $table_name, '%module_name' => $module_name));
      drupal_set_message($message, 'error');
      watchdog('features_tables', $message);
    }
    else {
      // import table from code to database
      if (!empty($table['data'])) {
        foreach ($table['data'] as $row) {
          databasetables_import_table_row($table_name, $row, $module_name);
        }
      }
    }
  }
  else {
    // error: schema is different:
    $message = t("Could not rebuild table '%table_name' from feature '%module_name': The schema for table '%table_name' in the database is different from the schema used in '%module_name'", array('%table_name' => $table_name, '%module_name' => $module_name));
    drupal_set_message($message, 'error');
    watchdog('features_tables', $message);
  }
}


/*
 * Import single row data to a table
 *
 * @param string $table_name
 *   The name of the table to import row data to
 * @param object &$row
 *   The row data object, a associative array of field keys and values
 * @param string $module_name
 *   The name of the feature module whose components should be rebuilt.
 */
function databasetables_import_table_row($table_name, $row, $module_name) {
 
  // store row into database
  
  // but we can't use...
  // $result = drupal_write_record($table_name, $row);
  // here, since it alters serial/autoincrement values on its own, 
  // which we don't want to happen here!

  $row['!table'] = $table_name;
  drupal_alter('features_tables_import_row', $row);
  unset($row['!table']);

  // so we do it manually and simple:
  $fields = array_keys($row);
  $values = array_values($row);
  $query = "INSERT INTO {" . $table_name . "} (" . implode(', ', $fields) . ") VALUES (" . db_placeholders($values, $type = 'varchar') . ")";
  $result = db_query($query, $values);
  $error = db_error();

  if ($error || ($result === FALSE)) {
    // error: drupal_write_record failed:
    $message = t("Could not rebuild table '%table_name' from feature '%module_name': drupal_write_record failed for the following row with the following error:\n\n ". $var_export($row, TRUE) ." \n\n $error", array('%table_name' => $table_name, '%module_name' => $module_name));
    drupal_set_message($message, 'error');
    watchdog('features_tables', $message);
  }

}


/*
 * Reduce a table schema definition and prepare it for comparison by removing irrelevant definitions
 *
 * @param array $table_schema
 *   The table schema definition as structured schema array
 * @return array
 *   The reduced table schema definition as structured schema array
 */
function databasetables_reduce_tableschema($table_schema) {
  unset($table_schema['description']);
  unset($table_schema['indexes']);
  foreach ($table_schema['fields'] as $key => $field) {
    unset($table_schema['fields'][$key]['default']);
    unset($table_schema['fields'][$key]['description']);
    unset($table_schema['fields'][$key]['disp-width']);
  }
  return $table_schema;
}








// Common or stub Implementations of hooks introduced by this module







/*
 * Implementation of hook_features_tables_export_preprocess();
 *
 * Called just before generating serialized export data from the database
 *
 * @param array $module_name
 *   The name of the feature module the export is generated for
 * @param array $table_names
 *   An array with (base-) names of tables that are going to be exported
 *
 */
function features_tables_features_tables_export_preprocess($module_name, $table_names) {

}


/*
 * Implementation of hook_features_tables_export_postprocess();
 *
 * Called right after generating serialized export data from the database
 *
 * @param array $module_name
 *   The name of the feature module the export is generated for
 * @param array $table_names
 *   An array with (base-) names of tables that are going to be exported
 *
 */
function features_tables_features_tables_export_postprocess($module_name, $table_names) {

}


/*
 * Implementation of hook_features_tables_export_row_alter();
 *
 * Called to alter a record/row retrieved from the database right before it is exported to a feature.
 *
 * @param array $row
 *   An array by reference with fieldname=>fieldvalue pairs which can be altered, unset or extended
 *   The table name is given in $row['!table']
 *
 */
function features_tables_features_tables_export_row_alter(&$row) {
  // don't import/export languages.javascript column
  if ($row['!table'] == 'languages') unset($row['javascript']);
}

                         
/*
 * Implementation of hook_features_tables_import_preprocess();
 *
 * Called just before importing serialized export data from a feature into the database
 *
 * @param array $module_name
 *   The name of the feature module the export is generated for
 * @param array $table_names
 *   An array with (base-) names of tables that are going to be exported
 *
 */
function features_tables_features_tables_import_preprocess($module_name, $table_names) {

}


/*
 * Implementation of hook_features_tables_import_postprocess();
 *
 * Called right after importing serialized export data from a feature to the database
 *
 * @param array $module_name
 *   The name of the feature module the export is generated for
 * @param array $table_names
 *   An array with (base-) names of tables that are going to be exported
 *
 */
function features_tables_features_tables_import_postprocess($module_name, $table_names) {
  // if table 'languages' was among imported tables ...
  if (in_array('languages', $table_names)) {
    // ... do as locale_languages_overview_form_submit in includes/locale.inc and ...
    // clear cache after language settings import
    cache_clear_all('*', 'cache_page', TRUE);
  }
}


/*
 * Implementation of hook_features_tables_import_row_alter();
 *
 * Called to alter a record/row retrieved from a feature just before it is imported into the databsase.
 *
 * @param array $row
 *   An array by reference with fieldname=>fieldvalue pairs which can be altered, unset or extended
 *   The table name is given in $row['!table']
 *
 */
function features_tables_features_tables_import_row_alter(&$row) {
  // don't import/export languages.javascript column
  if ($row['!table'] == 'languages') unset($row['javascript']);
}


