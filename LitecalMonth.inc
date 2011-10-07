<?php

/**
 * A month class. Manages a set of timespans which represent weeks of
 * the month and a single set of items which may be displayed in those
 * timespans.
 */
class LitecalMonth {
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
        $timespan->add($item->from, $item->to, $item->id, $item->data);
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
        if (!$this->options['day_overlay']) {
          $timespan_rows[] = $rendered;
        }
      }
      
      if ($this->options['day_overlay']) {
        $timespan->built = array();
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
