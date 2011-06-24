<?php

class litecal_handler_argument_date extends views_handler_argument_date {

  /**
   * The associated views query object.
   *
   * @var SearchApiViewsQuery
   */
  public $query;

  /**
   * Get granularity and use it to create the formula and a format
   * for the results.
   */
  function init(&$view, &$options) {
    parent::init($view, $options);

    // Identify the type of display we're using.
    $this->display_handler = $view->display_handler->definition['handler'];

    // Add a date handler.
    module_load_include('inc', 'date_api', 'date_api_sql');
    $this->date_handler = new date_sql_handler(DATE_UNIX);

    if (!empty($this->definition['field_name'])) {
      $field = field_info_field($this->definition['field_name']);
      if (!empty($field) && !empty($field['type'])) {
        $this->date_handler->date_type = $field['type'];
      }
    }
    $this->date_handler->granularity = $this->options['granularity'];

    $this->format = $this->date_handler->views_formats($this->date_handler->granularity, 'display');
    $this->sql_format = $this->date_handler->views_formats($this->date_handler->granularity, 'sql');

    // $this->arg_format is the format the parent date handler will use to create a default argument.
    //$this->arg_format = $this->format();

    if (empty($this->view->date_info)) {
      $this->view->date_info = new stdClass();
    }
    if (empty($this->view->date_info->date_fields)) {
      $this->view->date_info->date_fields = array();
    }
    $this->view->date_info->date_fields = array_merge($this->view->date_info->date_fields, $this->options['date_fields']);
  }

  /**
   * Determine if the argument can generate a breadcrumb
   *
   * @return boolean
   */
  // @todo Change and implement set_breadcrumb()?
  public function uses_breadcrumb() {
    return FALSE;
  }

  /**
   * Provide a list of default behaviors for this argument if the argument
   * is not present.
   *
   * Override this method to provide additional (or fewer) default behaviors.
   */
  /*
  public function default_actions($which = NULL) {
    $defaults = array(
      'ignore' => array(
        'title' => t('Display all values'),
        'method' => 'default_ignore',
        'breadcrumb' => TRUE, // generate a breadcrumb to here
      ),
      'not found' => array(
        'title' => t('Hide view / Page not found (404)'),
        'method' => 'default_not_found',
        'hard fail' => TRUE, // This is a hard fail condition
      ),
      'empty' => array(
        'title' => t('Display empty text'),
        'method' => 'default_empty',
        'breadcrumb' => TRUE, // generate a breadcrumb to here
      ),
      'default' => array(
        'title' => t('Provide default argument'),
        'method' => 'default_default',
        'form method' => 'default_argument_form',
        'has default argument' => TRUE,
        'default only' => TRUE, // this can only be used for missing argument, not validation failure
      ),
    );

    if ($which) {
      if (!empty($defaults[$which])) {
        return $defaults[$which];
      }
    }
    else {
      return $defaults;
    }
  }
  */

  /**
   * Set the empty argument value to the current date,
   * formatted appropriately for this argument.
   */
  function get_default_argument($raw = FALSE) {
    if (!$raw && $this->options['default_argument_type'] == 'date') {
      $granularity = $this->options['granularity'];
      if ($granularity == 'week') {
        $now = date_now();
        $week = date_week($now);
        return date_format($now, 'Y') .'-W'. $week;
      } 
      else {
        //return date($this->arg_format, REQUEST_TIME);
        return date(date_granularity_format($granularity), REQUEST_TIME);
      }
    }
    // Let the parent argument handle options like node created date.
    return parent::get_default_argument($raw);
  }

  /**
   * Default value for the date_fields option.
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['year_range'] = array('default' => '-3:+3', 'export' => 'export_plugin');
    $options['granularity'] = array('default' => 'month', 'export' => 'export_plugin');
    $options['default_argument_type'] = array('default' => 'date', 'export' => 'export_plugin');

    $options['date_fields'] = array('default' => array());
    $options['date_method'] = array('default' => 'OR', 'export' => 'export_plugin');
    $options['date_group'] = array('default' => 'date', 'export' => 'export_plugin');
    return $options;
  }

  /**
   * Make sure our custom options get exported.
   * Handle the options we know about, pass the rest to the parent plugin.
   */
  function export_plugin($indent, $prefix, $storage, $option, $definition, $parents) {
    $output = '';
    if (in_array($option, array('date_method', 'date_group'))) {
      $name = $this->options[$option];
      $output .= $indent . $prefix . "['$option'] = '$name';\n";
      return $output;
    }

    if (in_array($option, array('year_range', 'granularity', 'default_argument_type'))) {
      $name = $this->options[$option];
      $output .= $indent . $prefix . "['$option'] = '$name';\n";
      return $output;
    }
    return parent::export_plugin($indent, $prefix, $storage, $option, $definition, $parents);
  }

  /**
   * Add a form element to select date_fields for this argument.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $options = $this->date_handler->date_parts();
    unset($options['second'], $options['minute']);
    $options += array('week' => t('Week', array(), array('context' => 'datetime')));
    $form['granularity'] = array(
      '#title' => t('Granularity'),
      '#type' => 'radios',
      '#options' => $options,
      '#default_value' => $this->options['granularity'],
      '#multiple' => TRUE,
      '#description' => t("Select the type of date value to be used in defaults, summaries, and navigation. For example, a granularity of 'month' will set the default date to the current month, summarize by month in summary views, and link to the next and previous month when using date navigation."),
    );

    $form['year_range'] = array(
      '#title' => t('Date year range'),
      '#type' => 'textfield',
      '#default_value' => $this->options['year_range'],
      '#description' => t("Set the allowable minimum and maximum year range for this argument, either a -X:+X offset from the current year, like '-3:+3' or an absolute minimum and maximum year, like '2005:2010' . When the argument is set to a date outside the range, the page will be returned as 'Page not found (404)' ."),
    );

    $fields = _litecal_date_views_fields($this->view->base_table);
    $options = array();
    foreach ($fields['name'] as $name => $field) {
      $options[$name] = $field['label'];
    }

    $form['date_fields'] = array(
      '#title' => t('Date field(s)'),
      '#type' => 'checkboxes',
      '#options' => $options,
      '#default_value' => $this->options['date_fields'],
      '#multiple' => TRUE,
      '#description' => t("Select one or more date fields to filter with this argument. Do not select both the 'From date' and 'To date' for date fields, only one of them is needed."),
    );
    $form['date_method'] = array(
      '#title' => t('Method'),
      '#type' => 'radios',
      '#options' => array('OR' => t('OR'), 'AND' => t('AND')),
      '#default_value' => $this->options['date_method'],
      '#description' => t('Method of handling multiple date fields in the same query. Return items that have any matching date field (date = field_1 OR field_2), or only those with matches in all selected date fields (date = field_1 AND field_2). '),
      );
    $form['date_group'] = array(
      '#type' => 'hidden',
      '#value' => $this->options['date_group'],
    );
  }

  function options_validate(&$form, &$form_state) {
    // It is very important to call the parent function here:
    parent::options_validate($form, $form_state);

    if (!preg_match('/^(?:\-[0-9]{1,4}|[0-9]{4}):(?:[\+|\-][0-9]{1,4}|[0-9]{4})$/', $form_state['values']['options']['year_range'])) {
      form_error($form['year_range'], t('Date year range must be in the format -9:+9, 2005:2010, -9:2010, or 2005:+9'));
    }

    if ($form_state['values']['form_id'] == 'views_ui_config_item_form') {
      $check_fields = array_filter($form_state['values']['options']['date_fields']);
      if (empty($check_fields)) {
        form_error($form['date_fields'], t('You must select at least one date field for this argument.'));
      }
    }
  }

  function options_submit(&$form, &$form_state) {
    // It is very important to call the parent function here:
    parent::options_submit($form, $form_state);
    if ($form_state['values']['form_id'] == 'views_ui_config_item_form') {
      $form_state['values']['options']['date_fields'] = array_filter($form_state['values']['options']['date_fields']);
    }
  }

  function get_query_fields() {
    $fields = _litecal_date_views_fields($this->view->base_table);
    $fields = $fields['name'];
    $this->query_fields = array();
    foreach ($this->options['date_fields'] as $delta => $name) {
      if (array_key_exists($name, $fields) && $field = $fields[$name]) {
        $date_handler = new date_sql_handler($field['sql_type'], date_default_timezone());
        $date_handler->granularity = $this->options['granularity'];
        date_views_set_timezone($date_handler, $this, $field);
        $this->query_fields[] = array('field' => $field, 'date_handler' => $date_handler);
      }
    }
  }

  /**
   * Inject a test for valid date range before the regular query.
   * Override the parent query to be able to control the $group.
   */
  function query($group_by = FALSE) {
    $this->date_range = $this->date_handler->arg_range($this->argument);
    $this->min_date = $this->date_range[0];
    $this->max_date = $this->date_range[1];

    // Need to adjust the timezones of the generated date range to match the
    // timezone of the dates stored in the database.
    $this->min_date->setTimeZone(new DateTimeZone('UTC'));
    $this->max_date->setTimeZone(new DateTimeZone('UTC'));

    // Date api ranges all go to the 59th second of the 59th hour of the last
    // day in the range EXCEPT for week granularity (see date_week_range()).
    // We need to adjust the max date to counter this..
    $this->granularity = $this->date_handler->arg_granularity($this->argument);
    if ($this->granularity === 'week') {
      date_modify($this->max_date, '-1 second');
    }

    // Check to see if the provided argument is within the allowed range.
    if (!$this->within_range()) {
      return;
    }

    // Parsing the argument passed in url format.
    $block_identifier = date_block_identifier($this->view);
    if (!empty($this->view->block_identifier) || isset($_GET[$block_identifier])) {
      // Retrieve the block arguments in a way that will work for
      // urls like user/%/calendar/2009-04.
      if (!empty($_GET[$block_identifier])) {
        $path_args = explode('/', $this->view->get_path());
        $mini_args = explode('/', $_GET[$block_identifier]);
        foreach ($path_args as $pos => $key) {
          if ($path_args[$pos] != '%') {
            unset($mini_args[$pos]);
          }
        }
        // Get rid of gaps in the array caused by embedded args.
        $mini_args = array_values($mini_args);
        $this->view->args = $mini_args;
      }
      $i = 0;
      foreach ($this->view->argument as $argument) {
        if ($argument->field == 'date_argument_litecal') {
          $this->argument = $this->view->args[$argument->position];
          break;
        }
        $i++;
      }
    }

    $this->get_query_fields();
    if (!empty($this->query_fields)) {
      // TODO: This code assumes that there will be TWO (and only two) date 
      // fields provided and that they will be in the order: 'from', 'to'.
      foreach ($this->query_fields as $count => $query_field) {
        $field = $query_field['field'];
        $this->real_field = $field['real_field'];

        // TODO: Determine which of the following are really necessary.
        $this->date_handler = $query_field['date_handler'];
        $this->table = $field['table_name'];
        $this->date_handler->granularity = $this->date_handler->arg_granularity($this->argument);
        $this->format = $this->date_handler->views_formats($this->date_handler->granularity, 'display');
        $this->sql_format = $this->date_handler->views_formats($this->date_handler->granularity, 'sql');

        $this->ensure_my_table();

        // Begin creating our sub-filters (AND'ed).
        $filter = $this->query->createFilter();
        $filter->condition($this->real_field, $this->min_date, '>=');
        $filter->condition($this->real_field, $this->max_date, '<=');

        $filters[] = $filter;
      }

      // OR our sub-filters.
      $filter = $this->query->createFilter('OR');
      $filter->filter($filters[0]);
      $filter->filter($filters[1]);

      $this->query->filter($filter);
    }
  }

  /**
   * Get the title this argument will assign the view, given the argument.
   *
   * This usually needs to be overridden to provide a proper title.
   */
  public function title() {
    return t('Search @field for "@arg"', array('@field' => $this->definition['title'], '@arg' => $this->argument));
  }

  /**
   * Add a callback to determine if we have moved outside the valid date range for this argument.
   */
  private function within_range() {
    if (empty($this->argument)) {
      return;
    }

    $this->limit = date_range_years($this->options['year_range']); 
    $group = !empty($this->options['date_group']) ? $this->options['date_group'] : 0;

    // See if we're outside the allowed date range for our argument.
    if (date_format($this->min_date, 'Y') < $this->limit[0] || date_format($this->max_date, 'Y') > $this->limit[1]) {
      $this->forbid = TRUE;
      // TODO: Ideally we don't want to query solr at all, but at a minimum we 
      //       need to do an equivalent to this.
      //$this->query->add_where_expression($group, "0=1", array());
      return FALSE;
    }
    return TRUE;
  }
}
