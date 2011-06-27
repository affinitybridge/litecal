<?php

/**
 * Theme a timespan item.
 */
function theme_litecal_timeitem($args) {
  $attr['class'] = 'litecal-item';
  $attr['class'] .= $args['item']->starts ? ' starts' : '';
  $attr['class'] .= $args['item']->ends ? ' ends' : '';
  $attr['class'] .= " {$args['orientation']}";

  switch ($args['orientation']) {
    case 'vertical':
      break;
    case 'horizontal':
      $left = number_format($args['item']->start / $args['granularity'] * 100, 2);
      $width = number_format($args['item']->size / $args['granularity'] * 100, 2);
      $attr['style'] = "left:{$left}%; width:{$width}%;";
      break;
  }

  $data = "";
  if (is_array($args['item']->data)) {
    foreach ($args['item']->data as $field) {
      $data .= $field;
    }
  } else {
    $data = $args['item']->data;
  }

  if (module_exists('crayon')) {
    $attr['class'] .= ' crayon-anchor crayon-'. crayon_generate_index($args['item']->id);
    $popup = theme('crayon_popup', array($data, $args['item']->id));
  }
  else {
    $attr['class'] .= ' color';
    $popup = '';
  }

  if (!empty($item->url)) {
    return l("$popup<span class='litecal-data'>{$data}</span>", $args['item']->url, array('attributes' => $attr, 'html' => TRUE));
  }
  else {
    $attr = drupal_attributes($attr);
    return "<div {$attr}>$popup<span class='litecal-data'>{$data}</span></div>";
  }
}

/**
 * Theme all timeslots for a given timespan.
 */
function theme_litecal_timeslots($args) {
  $unit = $args['timespan']->unit;
  switch ($unit) {
    case 'days':
      $format = 'j';
      break;
  }

  $slots = array();
  $date = clone $args['timespan']->from;

  for ($i = 0; $i < $args['timespan']->granularity; $i++) { 
    $slot_items = (isset($args['slot_items'][$i])) ? $args['slot_items'][$i] : array();
    $slots[] = theme("litecal_timeslot_$unit", array('timespan' => $args['timespan'], 'start' => $i, 'date' => $date, 'format' => $format, 'quickadd' => $args['quickadd'], 'slot_items' => $slot_items));
    date_modify($date, "+1 $unit");
  }
  return $slots;
}

/**
 * Theme a single timeslot.
 */
function theme_litecal_timeslot_days($args) {
  $timespan = $args['timespan'];
  $start = $args['start'];
  $date = $args['date'];
  $format = $args['format'];
  $quickadd = $args['quickadd'];

  $add = '';
  $attr = array('style' => '');
  $link_attr = array('class' => 'label');

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

  // Classes
  $attr['class'] = 'litecal-slot rows-'. count($timespan->built);

  // Add class for today's slot.
  static $today;
  $today = !isset($today) ? date_format_date(date_now(), 'custom', 'Y-m-d') : $today;
  if ($today == date_format_date($date, 'custom', 'Y-m-d')) {
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
  elseif (!empty($args['slot_items'])) {
    $date_arr = $date->toArray();
    $date_arr['month'] = ($date_arr['month'] < 10) ? "0{$date_arr['month']}" : $date_arr['month'];
    $date_arr['day'] = ($date_arr['day'] < 10) ? "0{$date_arr['day']}" : $date_arr['day'];
    // TODO: This path is hard-coded, need to pass as an argument from the view.
    $link_attr['href'] = "calendar/{$date_arr['year']}-{$date_arr['month']}-{$date_arr['day']}/nojs";
    $link_attr['class'] = "label use-dialog use-ajax has-event";
  }
  $link_attr = drupal_attributes($link_attr);

  $formatted = date_format_date($date, 'custom', $format);

  return "<div {$attr}>
    <a $link_attr>{$add}<span class='num'>{$formatted}</span></a>
  </div>";
}

/**
 * Theme a header, like days of the week for a month.
 */
function theme_litecal_header($args) {
  $attr = array();

  switch ($args['orientation']) {
    case 'vertical':

      break;
    case 'horizontal':
      $left = number_format($args['start'] / $args['granularity'] * 100, 2);
      $width = number_format(1 / $args['granularity'] * 100, 2);
      $attr = array('style' => "left:{$left}%; width:{$width}%;");
      break;
  }
  $attr = drupal_attributes($attr);

  return "<div class='litecal-label' {$attr}>{$args['label']}</div>";
}
