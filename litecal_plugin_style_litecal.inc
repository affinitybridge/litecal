<?php
/**
 * @file
 * Contains the list style plugin.
 */

/**
 * Style plugin to render each item in an ordered or unordered list.
 *
 * @ingroup views_style_plugins
 */
class litecal_plugin_style_litecal extends views_plugin_style {

  /**
   * Set default options.
   */
  function option_definition() {
    $options = parent::option_definition();

    $options['style'] = array('default' => 'full');
    $options['colorby'] = array('default' => 0);
    $options['quickadd'] = array('default' => 0);

    $options['date_field'] = array('default' => 'created');

    if (module_exists('dialog')) {
      $options['day_overlay'] = array('default' => FALSE);
    }

    return $options;
  }

  /**
   * Extend the options form.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    // Style types
    $options = array(
      'full' => t('Full calendar'),
      'compact' => t('Compact calendar'),
    );
    $form['style'] = array(
      '#title' => t('Display style'),
      '#description' => t('Choose the display style for this litecal.'),
      '#type' => 'select',
      '#options' => $options,
      '#default_value' => $this->options['style'],
    );

    // Style types
    $fields = array(0 => '<'. t('None') .'>');
    foreach ($this->view->display_handler->get_option('fields') as $field => $definition) {
      $fields[$field] = !empty($definition['label']) ? $definition['label'] : $field;
    }
    $form['colorby'] = array(
      '#title' => t('Color by'),
      '#description' => t('Choose a field to be used for coloring items.'),
      '#type' => 'select',
      '#options' => $fields,
      '#default_value' => $this->options['colorby'],
    );    

    // Quick add type
    // TODO: THIS IS STILL BROKE
    /*
    if (module_exists('prepopulate')) {
      $nodetypes = node_type_get_types();
      $options = array(0 => '---'. t('Disabled') .'---');
      foreach (content_types() as $type => $info) {
        foreach ($info['fields'] as $field) {
          if ($field['type'] == 'date') {
            $options["{$field['type_name']}:{$field['field_name']}"] = check_plain($nodetypes[$field['type_name']]->name);
          }
        }
      }
      $form['quickadd'] = array(
        '#title' => t('Quickadd type'),
        '#description' => t('Choose the content type to use for quick event add links.'),
        '#type' => 'select',
        '#options' => $options,
        '#default_value' => $this->options['quickadd'],
      );
    }
    */

    $options = array('created' => t('Post date (default)'));
    foreach (field_info_fields() as $field_id => $field) {
      if ($field['type'] == 'date') {
        foreach ($field['bundles'] as $bundle => $types) {
          foreach ($types as $type) {
            $options["$field_id"] = "$field_id ($type)";
          }
        }
      }
    }
    $form['date_field'] = array(
      '#title' => t('Date field'),
      '#description' => t('Choose a date field to provide a value to be used to position the result in the calendar.'),
      '#type' => 'select',
      '#options' => $options,
      '#default_value' => $this->options['date_field'],
    );

    if (module_exists('dialog')) {
      $form['day_overlay'] = array(
        '#title' => t('Day view overlay'),
        '#description' => t('Render day listings to be presented within an overlay when the day has been selected.'),
        '#type' => 'checkbox',
        '#default_value' => $this->options['day_overlay'],
      );
    }
  }

  /**
   * Validate options.
   */
  function options_validate(&$form, &$form_state) {
    if (!$this->isValid()) {
      drupal_set_message(t('The litecal style requires you to add a <strong>Date argument</strong> to your view.'), 'error');
    }
  }

  function isValid() {
    $valid = FALSE;

    // Find argument handler
    $arguments = $this->view->display_handler->get_option('arguments');
    foreach ($arguments as $id => $handler) {
      if ($id == 'date_argument_search_api_dateextender'
        || $id == 'date_argument') {
        $valid = TRUE;
        break;
      }
    }

    return $valid;
  }

  /**
   * Template preprocessor.
   */
  function preprocess(&$vars) {
    drupal_add_css(drupal_get_path('module', 'litecal') .'/litecal.css');

    $view = $vars['view'];
    $options = $view->style_plugin->options;
    $handler = $view->style_plugin;

    $links = array();

    // Find the date_api argument handler.
    $arg_pos = 0;
    $args = isset($view->args) ? $view->args : array();

    $date_argument_handler = NULL;
    foreach ($view->argument as $argument => $handler) {
      // TODO: This probably isn't a great value to use...
      // Possibly use $handler->field as an alternative.
      if ($handler->definition['group'] === 'Date') {
        $date_argument_handler = $handler;
        break;
      }
      $arg_pos++;
    }

    $valid_argument = FALSE;
    if (!is_null($date_argument_handler)) {
      // This only works if the argument's default value has been set to
      // "current_date".
      // TODO: Need to add extra handling here.
      $valid_argument = TRUE;

      $arg = $date_argument_handler->argument;
      $arg_granularity = $date_argument_handler->granularity;
      $arg_from = $handler->min_date;
      $arg_to = $handler->max_date;
    }

    if (!$valid_argument) {
      $arg = date("Y-m", REQUEST_TIME);
      $handler = new date_sql_handler();
      $date_range = $handler->arg_range($arg);

      $arg_granularity = "month";
      $arg_from = $date_range[0];
      $arg_to = $date_range[1];
    }

    $date = clone $arg_from;
    $date_arr = $date->toArray();
    $year = $date_arr['year'];
    $month = $date_arr['month'];
    $day = $date_arr['day'];
    $week = date_week($date->format('Y-m-d'));
    $no_weeks = date_weeks_in_year($year);


    // Generate the right calendar type based on date granularity
    // TODO: This could definitely be made prettier.
    switch ($arg_granularity) {
      case 'month':
        // Generate Prev / Next links
        $next_month = ($month == 12) ? 1 : $month + 1;
        $next_month = ($next_month < 10) ? "0$next_month" : $next_month;
        $next_year = ($month == 12) ? $year + 1 : $year;

        $prev_month = ($month == 1) ? 12 : $month - 1;
        $prev_month = ($prev_month < 10) ? "0$prev_month" : $prev_month;
        $prev_year = ($month == 1) ? $year - 1 : $year;

        $args[$arg_pos] = "{$prev_year}-{$prev_month}";
        $link_prev = array('title' => t('Previous'), 'href' => $view->get_url($args));

        $args[$arg_pos] = "{$next_year}-{$next_month}";
        $link_next = array('title' => t('Next'), 'href' => $view->get_url($args));

        if ($options['day_overlay']) {
          $options['url'] = $view->get_url();
        }
        $litecal = new litecal_month($arg_from, $arg_to, $options);
        break;

      case 'week':
        $next_week = ($week == $no_weeks) ? 1 : $week + 1;
        $next_year = ($next_week == 1) ? $year + 1 : $year;

        $prev_week = ($week == 1) ? date_weeks_in_year($year - 1) : $week - 1;
        $prev_year = ($week == 1) ? $year - 1 : $year;

        $args[$arg_pos] = "{$prev_year}-W{$prev_week}";
        $link_prev = array('title' => t('Previous'), 'href' => $view->get_url($args));

        $args[$arg_pos] = "{$next_year}-W{$next_week}";
        $link_next = array('title' => t('Next'), 'href' => $view->get_url($args));

        $litecal = new LitecalWeek($arg_from, $arg_to, $options);
        break;

      case 'day':
        // Generate Prev / Next links
        $next_day = ($day == count(date_days(TRUE, $month))) ? 1 : $day + 1;
        $next_day = ($next_day < 10) ? "0$next_day" : $next_day;
        $next_month = ($next_day == 1) ? $month + 1 : $month;
        $next_month = ($next_month > 12) ? 1 : $next_month;
        $next_month = ($next_month < 10) ? "0$next_month" : $next_month;
        $next_year = ($next_day == 1 && $next_month == 1) ? $year + 1 : $year;

        $prev_day = ($day == 1) ? count(date_days(TRUE, $month - 1)) : $day - 1;
        $prev_day = ($prev_day < 10) ? "0$prev_day" : $prev_day;
        $prev_month = ($day == 1) ? $month - 1 : $month;
        $prev_month = ($prev_month == 0) ? 12 : $month;
        $prev_month = ($prev_month < 10) ? "0$prev_month" : $prev_month;
        $prev_year = ($day == 1 && $month == 1) ? $year - 1 : $year;

        $args[$arg_pos] = "{$prev_year}-{$prev_month}-{$prev_day}";
        $link_prev = array('title' => t('Previous'), 'href' => $view->get_url($args));
  
        $args[$arg_pos] = "{$next_year}-{$next_month}-{$next_day}";
        $link_next = array('title' => t('Next'), 'href' => $view->get_url($args));

        $litecal = new LitecalDay($arg_from, $arg_to, $options);
        break;
    }

    $links['prev'] = $link_prev;
    $args[$arg_pos] = "{$year}-{$month}-{$day}";
    $links['day'] = array('title' => t('Day'), 'href' => $view->get_url($args));
    $args[$arg_pos] = "{$year}-W{$week}";
    $links['week'] = array('title' => t('Week'), 'href' => $view->get_url($args));
    $args[$arg_pos] = "{$year}-{$month}";
    $links['month'] = array('title' => t('Month'), 'href' => $view->get_url($args));
    $links['next'] = $link_next;

    $vars['links'] = $links;

    $items = array();
    $aliases = array();
    $base = ($view->base_field === 'search_api_id') ? 'search_api' : $view->base_table;
    $date_field = $options['date_field'];
    $overlay = $options['day_overlay'];

    foreach ($view->result as $count => $result) {
      $date = array();
      // Special handling for each view base table source.
      // TODO: This could be a bit nicer.
      switch ($base) {
        case 'node':
          $default_colorby_field_id = $view->base_field;
          $entity = $result->_field_data['nid']['entity'];
          break;
        case 'search_api':
          $default_colorby_field_id = 'search_api_item_id';
          $entity = $result->entity;
          break;
      }

      // TODO: This needs to be made more generic.
      $url = "node/{$entity->occasion_parent[LANGUAGE_NONE][0]}";
      $id = !empty($colorby_field) ? $colorby_field->render($result) : $result->{$default_colorby_field_id};

      // Defualt date field selected (created).
      if ($date_field === 'created') {
        $date['value'] = $entity->{$date_field};
        $item = litecal_create_cal_item($date, $id, $this->rendered_fields[$count], $url);
        $items[] = $item;
      }

      // A Date module field has been selected.      
      else {
        $language = (isset($entity->language)) ? $entity->language : LANGUAGE_NONE;
        foreach ($entity->{$date_field}[$language] as $date) {
          $item = litecal_create_cal_item($date, $id, $this->rendered_fields[$count], $url);
          $items[] = $item;
        }
      }
    }

    if (isset($litecal)) {
      $litecal->add($items);
      $litecal->build();
      $vars = array_merge($vars, $litecal->built);
    }
  }
}

function litecal_create_cal_item($date, $id, $data, $url = NULL) {
  $timezone_db = (isset($date['timezone_db'])) ? $date['timezone_db'] : NULL;
  $item = new StdClass();
  $item->from = new DateObject($date['value'], $timezone_db);
  $item->to = isset($date['value2']) ? new DateObject($date['value2'], $date['timezone_db']) : clone $item->from;
  if ($timezone_db) {
    $item->from->setTimezone(new DateTimezone($date['timezone']));
    $item->to->setTimezone(new DateTimezone($date['timezone']));
  }
  $item->id = $id;
  $item->data = $data;
  if ($url) $item->url = $url;

  return $item;
}

/**
 * Our own date_difference function to give us a non absolute value difference.
 */
function litecal_date_difference($date1, $date2, $measure = 'seconds') {
  $modifier = date_format($date1, 'U') - date_format($date2, 'U') < 0 ? -1 : 1;
  //return date_difference($date1, $date2, $measure) * $modifier;
  return $date1->difference($date2, $measure) * $modifier;
}

/**
 * Boolean value of whether date1 is between date2 and date3.
 */
function litecal_date_between($date1, $date2, $date3) {
  $between = TRUE;
  $between = $between && (date_format($date1, 'U') - date_format($date2, 'U') >= 0);
  $between = $between && (date_format($date1, 'U') - date_format($date3, 'U') <= 0);
  return $between;
}

/**
 * A month class. Manages a set of timespans which represent weeks of
 * the month and a single set of items which may be displayed in those
 * timespans.
 */
class litecal_month {
  var $display_from;
  var $display_to;
  var $from;
  var $to;

  var $options = array();
  var $items = array();

  var $timespans;
  var $built;

  function __construct($from_date, $to_date, $options = array()) {
    $this->options = $options;

    $this->from = clone $from_date;
    $this->to = clone $to_date;

    $this->display_from = clone $from_date;
    $this->display_to = clone $to_date;

    // Get the day of the week for FROM, TO
    $from_weekday = date_format_date($this->from, 'custom', 'w');
    $to_weekday = date_format_date($this->to, 'custom', 'w');

    // Get the offset -- which must be %7. Note that the $to_offset must by < 7 as 
    // we should never add a full week.
    $_to_weekday_offset = variable_get('date_first_day', 1) - 1;
    $_to_weekday_offset = ($_to_weekday_offset >= 0) ? $_to_weekday_offset : (7 + $_to_weekday_offset);
    $from_offset = (7 - (variable_get('date_first_day', 1) - $from_weekday)) % 7;
    $to_offset = (7 - ($to_weekday - $_to_weekday_offset)) % 7;

    // Get display from/to dates of calendar
    date_modify($this->display_from, "-{$from_offset} days");
    date_modify($this->display_to, "+{$to_offset} days");

    // Generate timespans
    $current = clone $this->display_from;

    while (litecal_date_difference($this->display_to, $current, 'hours') > 0) {
      $timespan = new litecal_timespan($current, 7, 'days');

      // If the real calendar start is not the same as the display start,
      // We need to store it with the timespan.
      if (litecal_date_between($this->from, $timespan->from, $timespan->to)) {
        $timespan->real_from = clone $this->from;
      }
      // If the real calendar end is not the same as the display end,
      // We need to store it with the timespan.
      if (litecal_date_between($this->to, $timespan->from, $timespan->to)) {
        $timespan->real_to = clone $this->to;
      }

      $this->timespans[] = $timespan;
      date_modify($current, '+7 days');
    }
  }

  /**
   * Add an array of items to the calendar.
   */
  function add($items = array()) {
    $this->items = array_merge($this->items, $items);
    foreach ($items as $item) {
      foreach ($this->timespans as $timespan) {
        $timespan->add($item->from, $item->to, $item->id, $item->url, $item->data);
      }
    }
  }

  /**
   * Render items to HTML and store in structured array.
   */
  function build() {
    // Find and pass quickadd info if it is available.
    $quickadd = array();
    if (!empty($this->options['quickadd'])) {
      $split = explode(':', $this->options['quickadd']);
      $quickadd = array('type' => $split[0], 'field' => $split[1]);
    }

    // Render items and slots
    foreach ($this->timespans as $num => $timespan) {
      $timespan->build();
      $timespan_rows = array();
      $slot_items = array();
      foreach ($timespan->built as $timespan_row) {
        $rendered = array();
        foreach ($timespan_row as $k => $item) {
          // Extra processing for day overlay.
          if ($this->options['day_overlay']) {
            for ($slot = $item->start, $end = $item->end; $slot <= $end; $slot++) {
              $timespan->url = $this->options['url'];
              $slot_items[$slot][] = $item;
            }
          }
          // Regular litecal function.
          else {
            $rendered[] = theme('litecal_timeitem', array('item' => $item, 'granularity' => $timespan->granularity));
          }
        }
        $timespan_rows[] = $rendered;
      }
      
      $this->built['timespans'][$num]['class'] = ' rows-' . count($timespan_rows);
      $this->built['timespans'][$num]['rows'] = $timespan_rows;
      $this->built['timespans'][$num]['slots'] = theme('litecal_timeslots', array('timespan' => $timespan, 'quickadd' => $quickadd, 'slot_items' => $slot_items));
    }

    // Pass display style information on
    $display_style = !empty($this->options['style']) ? $this->options['style'] : 'full';
    $this->built['class'] = "litecal-{$display_style}";

    // Build header labels
    switch ($display_style) {
      case 'compact':
        $weekdays = date_week_days_ordered(date_week_days_abbr(TRUE, TRUE, 1));
        break;
      default:
        $weekdays = date_week_days_ordered(date_week_days(TRUE));
        break;
    }
    $total = count($weekdays);
    foreach ($weekdays as $num => $label) {
      $this->built['header'][] = theme('litecal_header', array('label' => $label, 'start' => $num, 'granularity' => $total));
    }
  }
}

/**
 * A timespan class. Represents any generalized slice of time where
 * event items can be displayed.
 */
class litecal_timespan {
  var $from;
  var $to;
  var $real_from;
  var $real_to;
  var $unit;
  var $granularity;

  var $extender;
  var $vacant = array();
  var $occupied = array();

  /**
   * Constructor.
   *
   * @param $from
   *   A date object that represents the start time of this timespan.
   * @param $granularity
   *   Int number of "slots" that are contained by the timespan.
   * @param $unit
   *   A dateAPI friendly unit string, e.g. "days"
   */
  function __construct($from, $granularity, $unit) {
    $this->from = clone $from;
    $this->to = clone $from;
    date_modify($this->to, "+{$granularity} {$unit}");

    // These values can be manipulated post initialization.
    $this->real_from = clone $this->from;
    $this->real_to = clone $this->to;

    $this->granularity = $granularity;
    $this->unit = $unit;

    $this->extender = new litecal_timeitem(0, $granularity - 1);
    $this->extender->y = 0;
  }

  /**
   * Converts an item's dates into start/end coordinates in the context
   * of this timespan.
   *
   * @param $from
   *   A date object that represents the start time of the item.
   * @param $to
   *   A date object that represents the end time of the item.
   */
  function convert($from, $to) {
    // Ensure the times are within the timespan
    $to_from = litecal_date_difference($to, $this->from, $this->unit);
    $from_to = litecal_date_difference($from, $this->to, $this->unit);
    if ($to_from >= 0 && $from_to < 0) {
      $x = 0;

      if (litecal_date_difference($from, $this->from) >= 0) {
        $start = litecal_date_difference($from, $this->from, $this->unit);
      }
      else {
        $start = 0;
      }

      if (litecal_date_difference($to, $this->to, $this->unit) < 0) {
        litecal_date_difference($to, $this->to, $this->unit);
        $end = litecal_date_difference($to, $this->from, $this->unit);
      }
      else {
        $end = $this->granularity - 1;
      }

      $item = new litecal_timeitem($start, $end, LITECAL_ITEM);
      $item->starts = litecal_date_difference($from, $this->from, $this->unit) >= 0 ? TRUE : FALSE;
      $item->ends = litecal_date_difference($to, $this->to, $this->unit) < 0 ? TRUE : FALSE;
      return $item;
    }
    return FALSE;
  }

  /**
   * Add an item to the timespan.
   *
   * @param $from
   *   A date object that represents the start time of the item.
   * @param $to
   *   A date object that represents the end time of the item.
   * @param $id
   *   An identifier for use in coloring items.
   * @param $url
   *   The url to link this item to.
   * @param $data
   *   Any additional data to be displayed in the item.
   */
  function add($from, $to, $id, $url = NULL, $data = array()) {
    $item = $this->convert($from, $to);
    if ($item) {
      $item->id = $id;
      $item->data = $data;
      $item->url = $url;

      foreach ($this->vacant as $k => $v) {
        if ($v->contains($item)) {
          // Set the item row and add to occupied array
          $item->y = $v->y;
          $this->occupied[] = $item;

          // Generate divided slots and add to vacancies
          $new_slots = $v->divide($item);
          unset($this->vacant[$k]);
          $this->vacant = array_merge($this->vacant, $new_slots);

          return TRUE;
        }
      }

      // If we get to this point, the item has not yet been added
      if ($this->extender->contains($item)) {
        // Use the extender row for the new item
        $item->y = $this->extender->y;
        $this->occupied[] = $item;

        // Generate divided slots and add to vacancies
        $new_slots = $this->extender->divide($item);
        $this->vacant = array_merge($this->vacant, $new_slots);

        // Increment the extender row
        $this->extender->y = $this->extender->y + 1;

        return TRUE;
      }
    }
  }

  /**
   * Render the items.
   */
  function build() {
    $rows = array();
    foreach ($this->occupied as $item) {
      $rows[$item->y][$item->start] = $item;
    }
    ksort($rows);
    foreach ($rows as $k => $v) {
      ksort($rows[$k]);
    }
    $this->built = $rows;
  }
}

/**
 * A timespan item class. Represents any continuous block of time in a
 * timespan. May be either an actual item (event) or a "vacant" space.
 */
class litecal_timeitem {
  // Coordinate data
  var $start;
  var $end;
  var $y;
  var $size;

  // Content + metadata
  var $id;
  var $url;
  var $data;
  var $type;

  /**
   * Constructor.
   *
   * @param $start
   *   Int start coordinate for this timespan item.
   * @param $end
   *   Int end coordinate for this timespan item.
   * @param $type
   *   Either LITECAL_ITEM or LITECAL_EMPTY.
   */
  function __construct($start, $end, $type = LITECAL_EMPTY) {
    $this->start = $start;
    $this->end = $end;
    $this->size = $end - $start + 1;
    $this->type = $type;
  }

  /**
   * Tests whether the given item (same class) fits within the current one.
   */
  function contains($item) {
    return ($item->start >= $this->start && $item->end <= $this->end) ? TRUE : FALSE;
  }

  /**
   * Divides the current item into up to two new vacant items.
   */
  function divide($item) {
    $divided = array();

    if ($this->contains($item)) {
      // If the item begins after me, create a new item from the excess
      // slots prior to the item.
      if ($item->start > $this->start) {
        $prior = new litecal_timeitem($this->start, $item->start - 1);
        $prior->y = $this->y;
        $divided[] = $prior;
      }

      // If the item ends before me, create a new item from the excess
      // slots after the item.
      if ($item->end < $this->end) {
        $post = new litecal_timeitem($item->end + 1, $this->end);
        $post->y = $this->y;
        $divided[] = $post;
      }

    }
    return $divided;
  }
}
