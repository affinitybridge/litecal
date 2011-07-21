<div class='litecal <?php print $class ?>'>

<?php if (!empty($header)) : ?>
<div class='litecal-header'>
<?php foreach ($header as $label): ?>
  <?php print $label; ?>
<?php endforeach; ?>
</div>
<?php endif; ?>

<?php if (!empty($timespans)) : ?>
<?php foreach ($timespans as $num => $timespan): ?>
  <?php
  $class = '';
  $class .= ($num == 0) ? ' timespan-first' : '';
  $class .= ($num == count($timespans) - 1) ? ' timespan-last' : '';
  $class .= (isset($timespan['class'])) ? $timespan['class'] : '';
  ?>
  <div class='litecal-timespan <?php print $class ?>'>

  <?php foreach ($timespan['slots'] as $slot): ?>
    <?php print $slot ?>
  <?php endforeach; ?>

  <?php foreach ($timespan['rows'] as $row): ?>
    <div class='litecal-row clearfix'>
      <?php foreach ($row as $item): ?>
        <?php print $item; ?>
      <?php endforeach; ?>
    </div>
  <?php endforeach; ?>

  </div>
<?php endforeach; ?>
<?php else : ?>
  <div>
    <p>There are no events occurring on this day.</p>
  </div>
<?php endif; ?>

</div>
