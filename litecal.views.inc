<?php

/**
 * Implementation of hook_views_data()
 */
function litecal_views_data() {
  $data = array();

  $tables = _get_search_api_index_base_tables();

  foreach ($tables as $base_table) {
    $data[$base_table]['date_argument_litecal'] = array(
      'group' => t('Date'),
      'title' => t('Litecal (!base_table)', array('!base_table' => $base_table)),
      'help' => t('Filter any Search API index (!base_table) date field by a date argument, using any common ISO date/period format (i.e. YYYY, YYYY-MM, YYYY-MM-DD, YYYY-W99, YYYY-MM-DD--P3M, P90D, etc). ', array('!base_table' => $base_table)),
      'argument' => array(
        'handler' => 'litecal_handler_argument_date',
        'empty field name' => t('Undated'),
      ),
    );  
  }

  return $data;
}
