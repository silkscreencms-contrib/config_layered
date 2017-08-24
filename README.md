Layered Configuration Driver (config_layered)
=============================================

This driver contains the Layered configuration driver for Silkscreen CMS.

To enable this driver, place it in the `drivers` directory in the root of the
Silkscreen CMS site or in the sites/[yoursite]/drivers directory.

This configuration storage driver combines several configuration objects in a
stack. When reading configuration data, the driver starts and the top of the
stack and searches each configuration object until the requested setting is
found. When writing, the driver starts at the top of the stack and looks for
the first mutable object, and writes the configuration there.

To configure this, first setup the various layers according to each driver's
directions. Then, create the layered configuration referencing the other
layers. The prefix for layered configuration URLs is `layered`.

For example:

```php
// Define a memory-based config object and a file-based config object.
$config_directories['memory'] = 'mem:mem_config';
$config_directories['active_files'] = 'file://files/config/active';

// Now layer the two.
$config_directories['active'] = 'layered:/memory/active_files';

// Create some memory configurations. (i.e., overrides.)
$settings['config']['mem_config'] = array(
  'system.core' => array(
    'site_name' => 'Overridden Site Name',
  ),
);
```

With this setup, reads for the `site_name` configuration setting in
`system.core` will come from the `memory` configuration object, but since the
memory-based configuration object is immutable, all writes will go to the
`active_files` configuration object, including writes for `site_name`.

License
-------

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.

Maintainers
-----------

- John Franklin (https://github.com/jlfranklin/)
