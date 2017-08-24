<?php

/**
 * Defines an interface for configuration storage controllers.
 *
 * Classes implementing this interface allow reading and writing configuration
 * data from and to the storage.
 */
class ConfigLayeredStorage implements ConfigStorageInterface {

  /**
   * The name of the storage class.
   * @var string
   */
  static $storage_class = 'Layered';

  protected $layers = array();

  protected $immutable = NULL;

  protected $mutableLayer = NULL;

  protected $initialized = FALSE;

  protected $data = array();

  public static function urlPrefix() {
    return 'layered';
  }

  public function isImmutable() {
    return $this->mutableLayer === NULL;
  }

  /**
   * {@inheritdoc}
   */
  public function initializeStorage() {
    foreach ($this->layers as $l) {
      $l->initializeStorage();
    }
    $this->initialized = TRUE;
  }

  public function __construct($url) {
    $matches = array();
    preg_match("/^layered:(\S*)$/", $url, $matches);
    $parts = explode('/', $matches[1]);
    foreach (array_filter($parts) as $part) {
      $this->layers[$part] = config_get_config_storage($part);
      if (!$this->mutableLayer && !$this->layers[$part]->isImmutable()) {
        $this->mutableLayer = $this->layers[$part];
      }
    }
  }

  /**
   * {@inheritdoc}
   */
  public function isInitialized() {
    return $this->initialized;
  }

  /**
   * {@inheritdoc}
   */
  public function exists($name) {
    foreach ($this->layers as $l) {
      if ($l->exists($name)) {
        return TRUE;
      }
    }
    return FALSE;
  }

  /**
   * {@inheritdoc}
   */
  public function read($name) {
    $data = array();
    foreach ($this->layers as $l) {
      if ($l->exists($name)) {
        $data += $l->read($name);
      }
    }
    return empty($data) ? FALSE : $data;
  }

  /**
   * {@inheritdoc}
   */
  public function readMultiple(array $names) {
    $list = array();
    foreach ($names as $name) {
      if ($data = $this->read($name)) {
        $list[$name] = $data;
      }
    }
    return $list;
  }

  /**
   * Writes configuration data to the storage.
   *
   * @param string $name
   *   The name of a configuration object to save.
   * @param array $data
   *   The configuration data to write.
   *
   * @return bool
   *   TRUE on success, FALSE in case of an error.
   */
  public function write($name, array $data) {
    if ($this->mutableLayer) {
      return $this->mutableLayer->write($name, $data);
    }
    throw new ConfigStorageException("Immutable object.");
  }

  /**
   * Fill the object with data.  This can be done once to initialize the object,
   * after which it will throw a ConfigStorageException.
   * @param array $data
   *   The configuration data.
   * @return bool
   *   TRUE on success.
   * @throws ConfigStorageException
   */
  public function writeMultiple(array $data) {
    if ($this->mutableLayer) {
      return $this->mutableLayer->writeMultiple($data);
    }

    // No mutable layer?  Throw an exception.    
    throw new ConfigStorageException("Immutable object.");
  }

  /**
   * Deletes a configuration object from the storage.
   *
   * @param string $name
   *   The name of a configuration object to delete.
   *
   * @return bool
   *   TRUE on success, FALSE otherwise.
   */
  public function delete($name) {
    if ($this->mutableLayer) {
      return $this->mutableLayer->delete($name);
    }
    return FALSE;
  }

  /**
   * Renames a configuration object in the storage.
   *
   * @param string $name
   *   The name of a configuration object to rename.
   * @param string $new_name
   *   The new name of a configuration object.
   *
   * @return bool
   *   TRUE on success, FALSE otherwise.
   */
  public function rename($name, $new_name) {
    $success = TRUE;
    foreach ($this->layers as $l) {
      if (!$l->isImmutable() && $l->exists($name)) {
        $success &= $l->rename($name, $new_name);
      }
    }
    return $success;
  }


  /**
   * Returns a timestamp indicating the last time a configuration was modified.
   *
   * @param string $name
   *   The name of a configuration object on which the time will be checked.
   *
   * @return int
   *   A timestamp indicating the last time the configuration was modified.
   */
  public function getModifiedTime($name) {
    foreach ($this->layers as $l) {
      if ($l->exists($name)) {
        return $l->getModifiedTime($name);
      }
    }
  }

  /**
   * Encodes configuration data into the storage-specific format.
   *
   * @param array $data
   *   The configuration data to encode.
   *
   * @return string
   *   The encoded configuration data.
   *
   * This is a publicly accessible static method to allow for alternative
   * usages in data conversion scripts and also tests.
   */
  public function encode($data) {
    $contents = backdrop_json_encode($data, TRUE);
    if ($contents === FALSE) {
      throw new ConfigStorageException(t('The configuration string could not be parsed.'));
    }
    return $contents;
  }

  /**
   * Decodes configuration data from the storage-specific format.
   *
   * @param string $raw
   *   The raw configuration data string to decode.
   *
   * @return array
   *   The decoded configuration data as an associative array.
   *
   * This is a publicly accessible static method to allow for alternative
   * usages in data conversion scripts and also tests.
   */
  public function decode($raw) {
    // Use json_decode() directly for efficiency.
    $contents = json_decode($raw, TRUE);
    if (is_null($contents)) {
      throw new ConfigStorageException('The configuration string could not be parsed.');
    }
    return $contents;
  }

  /**
   * Gets configuration object names starting with a given prefix.
   *
   * Given the following configuration objects:
   * - node.type.post
   * - node.type.page
   *
   * Passing the prefix 'node.type.' will return an array containing the above
   * names.
   *
   * @param string $prefix
   *   (optional) The prefix to search for. If omitted, all configuration object
   *   names that exist are returned.
   *
   * @return array
   *   An array containing matching configuration object names.
   */
  public function listAll($prefix = '') {
    $elements = array();
    foreach ($this->layers as $l) {
      $elements += array_fill_keys($l->listAll($prefix), TRUE);
    }
    return array_keys($elements);
  }

  /**
   * Deletes configuration objects whose names start with a given prefix.
   *
   * Given the following configuration object names:
   * - node.type.post
   * - node.type.page
   *
   * Passing the prefix 'node.type.' will delete the above configuration
   * objects.
   *
   * @param string $prefix
   *   (optional) The prefix to search for. If omitted, all configuration
   *   objects that exist will be deleted.
   *
   * @return boolean
   *   TRUE on success, FALSE otherwise.
   */
  public function deleteAll($prefix = '') {
    if ($this->mutableLayer) {
      return $this->mutableLayer->deleteAll($prefix);
    }
    throw new ConfigStorageException("Immutable object.");
  }

  /**
   * Import an archive of configuration files into the config storage managed
   * by this object.
   *
   * @param string $file_uri
   *   The URI of the tar archive file to import.
   *
   * @throws ConfigStorageException
   *
   * @return bool
   *   TRUE on success, FALSE otherwise.
   */
  public function importArchive($file_uri) {
    if ($this->mutableLayer) {
      return $this->mutableLayer->importArchive($file_uri);
    }
    throw new ConfigStorageException("Immutable object.");
  }

  /**
   * Export an archive of configuration files from the config storage managed
   * by this object.
   *
   * @param string $file_uri
   *   The URI of the tar archive file to create.
   *
   * @throws ConfigStorageException
   *
   * @return bool
   *   TRUE on success, FALSE otherwise.
   */
  public function exportArchive($file_uri) {
    throw new ConfigStorageException("Immutable object.");
  }
}
