<?php

/**
 * Biblio class.
 */
class Biblio extends Entity {

  public function __construct($values = array()) {
    parent::__construct($values, 'biblio');
  }

  /**
   * Override Entity:save().
   *
   * Clear biblio cache.
   */
  public function save() {
    $wrapper = entity_metadata_wrapper('biblio', $this);

    if (!$wrapper->biblio_status->value() && $wrapper->biblio_year->value()) {
      // If Year is populated and Status is not, the status should be Published.
      $wrapper->biblio_status->set('published');
    }

    $this->md5 = BiblioStyleBase::generateBiblioMd5($this);

    if (empty($this->_skip_cache)) {
      $this->cache = '';
      $this->cache_id = '';
    }

    parent::save();
  }

  /**
   * Get the text, rendered using a Biblio style plugin.
   *
   * @param $style_name
   *   The Biblio style plugin.
   * @param $options
   *   Optional; Array with settings specific to the select style name.
   * @param $skip_cache
   *   Optional; Determine if cache values should be skipped . Defaults to
   *   FALSE.
   */
  public function getText($style_name, $options = array(), $skip_cache = FALSE) {
    if (!$plugin = biblio_get_biblio_style($style_name)) {
      // @todo: Add own exception.
      throw new Exception(format_string('Biblio style "@style" does not exist.', array('@style' => $style_name)));
    }

    foreach ($plugin['assets']['js'] as $js_file) {
      drupal_add_js($js_file);
    }
    foreach ($plugin['assets']['css'] as $css_file) {
      drupal_add_css($css_file);
    }

    if (!variable_get('biblio_debug') && $output = $this->getCache($style_name, $options, $skip_cache)) {
      return $output;
    }

    $plugin['options'] = drupal_array_merge_deep($plugin['options'], $options);

    $class = ctools_plugin_load_class('biblio', 'biblio_style', $style_name, 'class');

    $biblio_style = new $class($plugin, $this);
    $output = $biblio_style->render();

    // @todo: We do this because the title might have link. Is there a nicer way?
    $this->title = strip_tags($this->title);

    if ($skip_cache) {
      return $output;
    }

    // Cache the output.
    $this->cache = $output;
    $this->cache_id = md5(serialize(array($style_name, $options)));

    // Make sure the cache isn't cleared on save.
    $this->_skip_cache = TRUE;

    $wrapper = entity_metadata_wrapper('biblio', $this);

    if (isset($wrapper->biblio_first_letter)) {
      // Assign first letter to ease grouping.
      $trimmed = trim(strip_tags($output), ' “');
      $wrapper->biblio_first_letter->set(strtoupper(drupal_substr($trimmed, 0, 1)));
    }

    $this->save();

    return $output;

  }

  /**
   * Helper function to get cached values.
   *
   * @param $style_name
   * @param array $options
   * @param bool $skip_cache
   */
  private function getCache($style_name, $options = array(), $skip_cache = FALSE) {
    if ($skip_cache) {
      return;
    }

    if (empty($this->cache)) {
      return;
    }

    // Try to get the values from the cache.
    $cache_id = md5(serialize(array($style_name, $options)));
    if ($this->cache_id != $cache_id) {
      return;
    }

    return $this->cache;
  }

  /**
   * Build the uri of the biblio.
   */
  function uri() {
    return array('path' => 'biblio/' . $this->bid);
  }
}
