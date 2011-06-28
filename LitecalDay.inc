<?php

/**
 * A month class. Manages a set of timespans which represent weeks of
 * the month and a single set of items which may be displayed in those
 * timespans.
 */
class LitecalDay {
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

    while (litecal_date_difference($this->display_to, $current, 'hours') > 0) {
      $timespan = new litecal_timespan($current, 1, 'hours');

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
      date_modify($current, '+60 minutes');
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
    $hours = date_hours('H', TRUE);
    $total = count($hours);
    foreach ($hours as $num => $label) {
      $headers[$num] = theme('litecal_header', array('label' => "$label:00", 'start' => $num, 'granularity' => $total, 'orientation' => 'vertical'));
    }

    // Getting current hour.
    $now = date_format_date(date_now(), 'custom', 'Y-m-d:H');

    $rendered_events = array();
    // Render items and slots
    foreach ($this->timespans as $num => $timespan) {
      $timespan->build();
      $timespan_rows = array();
      foreach ($timespan->built as $timespan_row) {
        $rendered = array();
        foreach ($timespan_row as $k => $item) {
          if (!isset($rendered_events[$item->id])) {
            $rendered[] = theme('litecal_timeitem', array('item' => $item, 'granularity' => $timespan->granularity, 'orientation' => 'vertical'));
            $rendered_events[$item->id] = $item;
          }
        }
        if (!empty($rendered)) $timespan_rows[] = $rendered;
      }
      // If no content for this row skip it.
      if (empty($timespan_rows)) {
        unset($this->built['timespans'][$num]);
        continue;
      }

      $this->built['timespans'][$num]['rows'] = $timespan_rows;
      $this->built['timespans'][$num]['slots'] = array($headers[$num]);

      // Add class for today's slot.
      // TODO: Adjust for timezone.
      if ($now == date_format_date($timespan->from, 'custom', 'Y-m-d:H')) {
        $this->built['timespans'][$num]['class'] .= ' litecal-slot-today';
      }

      static $zebra;
      $zebra = !isset($zebra) ? TRUE : !$zebra;
      $class = ($zebra) ? ' litecal-hour-even' : ' litecal-hour-odd';
      if (isset($this->built['timespans'][$num]['class'])) {
        $this->built['timespans'][$num]['class'] .= $class;
      } else {
        $this->built['timespans'][$num]['class'] = $class;
      }
    }

    // Pass display style information on
    $display_style = !empty($this->options['style']) ? $this->options['style'] : 'full';
    $this->built['class'] = "litecal-{$display_style} litecal-day";
  }
}

/**
 * Theme a single timeslot.
 */
function theme_litecal_timeslot_hours($args) {
  $timespan = $args['timespan'];
  $start = $args['start'];
  $date = $args['date'];
  $format = $args['format'];
  $quickadd = $args['quickadd'];

  $add = '';
  $attr = array('style' => '');
  $link_attr = array('class' => 'label');

/*
  // Position
  if ($start < $timespan->granularity - 1) {
    $attr['style'] .= ' left:'. number_format($start / $timespan->granularity * 100, 2) .'%;';
  }
  // We position last items differently since slots often use borders and need tight alignment.
  else {
    $attr['style'] .= ' right:0%;';
  }

  // Width
  $attr['style'] .= ' width:'. number_format(1 / $timespan->granularity * 100, 2) .'%';
*/
  // Classes
  $attr['class'] .= ' litecal-slot rows-'. count($timespan->built);

  // Add class for today's slot.
  static $now;
  $now = !isset($now) ? date_format_date(date_now(), 'custom', 'Y-m-d:H') : $now;
  if ($now == date_format_date($date, 'custom', 'Y-m-d:H')) {
    $attr['class'] .= ' litecal-slot-today';
  }

  // If this timeslot is outside of the timespan's real time range,
  // add a class so it can be displayed accordingly.
  if (!litecal_date_between($date, $timespan->real_from, $timespan->real_to)) {
    $attr['class'] .= ' litecal-slot-gutter';
  }

  $attr = drupal_attributes($attr);

  // Quickadd
  if (!empty($quickadd['type'])) {
    $type = str_replace('_', '-', $quickadd['type']);
    $item = menu_get_item("node/add/{$type}");
    if ($item && $item['access']) {
      $options = array('query' => "edit[{$quickadd['field']}][0][value][date]=". date_format_date($date, 'custom', 'm/d/Y') . 
        "&edit[{$quickadd['field']}][0][value][time]=" . date_format_date($date, 'custom', 'g:iA') ."&destination=" . $_GET['q']);
      $link_attr['href'] = url("node/add/{$type}", $options);
      $add = "<span class='add'>". t('+ Add') ."</span>";
    }
  }
  $link_attr = drupal_attributes($link_attr);

  $formatted = date_format_date($date, 'custom', $format);


  return "<div {$attr}>
    <span class='hour'>{$start}</span>
    <a $link_attr>{$add}<span class='num'>{$formatted}</span></a>
  </div>";
}
