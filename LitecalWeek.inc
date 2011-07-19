<?php

/**
 * A week class. Manages a set of timespans which represent days of a week and 
 * a single set of items which may be displayed in those timespans.
 */
class LitecalWeek {
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

    // Generate timespans
    $current = clone $from_date;

    while (litecal_date_difference($this->display_to, $current, 'days') > 0) {
      $timespan = new litecal_timespan($current, 1, 'days');

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
      date_modify($current, '+1 day');
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
    $days = date_week_days_ordered(date_week_days_abbr(TRUE));
    $total = count($days);
    $current = clone $this->from;
    foreach ($days as $num => $label) {
      $label = strtoupper($label) . '<br/>' . date_format_date($current, 'custom', 'M d');
      $headers[$num] = theme('litecal_header', array('label' => $label, 'start' => $num, 'granularity' => $total, 'orientation' => 'vertical'));

      date_modify($current, '+1 day');
    }

    // Getting current day.
    $now = date_now();
    $now->setTimezone(new DateTimeZone("UTC"));
    $now = date_format_date($now, 'custom', 'Y-m-d');

    $rendered_events = array();
    // Render items and slots
    foreach ($this->timespans as $num => $timespan) {
      $timespan->build();
      $timespan_rows = array();
      foreach ($timespan->built as $timespan_row) {
        $rendered = array();
        foreach ($timespan_row as $k => $item) {
          //if (!isset($rendered_events[$item->id])) {
            $rendered[] = theme('litecal_timeitem', array('item' => $item, 'granularity' => $timespan->granularity, 'orientation' => 'vertical'));
            $rendered_events[$item->id] = $item;
          //}
        }
        if (!empty($rendered)) $timespan_rows[] = $rendered;
      }

      $this->built['timespans'][$num]['rows'] = $timespan_rows;
      $this->built['timespans'][$num]['slots'] = array($headers[$num]);

      $class = "";

      // Add class for today's slot.
      if ($now == date_format_date($timespan->from, 'custom', 'Y-m-d')) {
        $class .= ' litecal-slot-today';
      }

      if (empty($timespan_rows)) {
        $class .= " clearfix";
      }

      static $zebra;
      $zebra = !isset($zebra) ? TRUE : !$zebra;
      $class .= ($zebra) ? ' litecal-day-even' : ' litecal-day-odd';
      if (isset($this->built['timespans'][$num]['class'])) {
        $this->built['timespans'][$num]['class'] .= $class;
      } else {
        $this->built['timespans'][$num]['class'] = $class;
      }
    }

    $headers = $this->options['headers'];
    $total = count($headers);
    $this->built['header'][] = theme('litecal_column_header', array(
      'label' => t('Date'),
      'attributes' => array(
        'class' => 'litecal-column-header-first litecal-header-date',
      )
    ));
    $count = 0;
    foreach ($headers as $fieldname => $label) {
      $attributes = array(
        'class' => 'litecal-header-' . str_replace('_', '-', $fieldname),
      );
      if ($count == $total - 1) $attributes['class'] .= " litecal-column-header-last";

      $this->built['header'][] = theme('litecal_column_header', array(
        'label' => $label,
        'attributes' => $attributes,
      ));
      $count++;
    }

    // Pass display style information on
    $display_style = !empty($this->options['style']) ? $this->options['style'] : 'full';
    $this->built['class'] = "litecal-{$display_style} litecal-week";
  }
}
