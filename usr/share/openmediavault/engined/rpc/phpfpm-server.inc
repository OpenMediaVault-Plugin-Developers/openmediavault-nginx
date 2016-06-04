<?php

/**
 * Copyright (C) 2014-2015 OpenMediaVault Plugin Developers.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

namespace OMV\Engined\Rpc;

use OMV\Config\ConfigObject;
use OMV\Config\Database;
use OMV\Rpc\ServiceAbstract;

class PhpFpmServer extends ServiceAbstract
{
    /** @var Database */
    private $database;

    /** @var string */
    private $dataModelPath = 'conf.service.phpfpm.pool';

    public function __construct(Database $database = null)
    {
        $this->database = $database ?: Database::getInstance();
    }

    /**
     * Get the name of the RPC service.
     *
     * @return string
     */
    public function getName()
    {
        return 'PhpFpm';
    }

    /**
     * Initialize the RPC service.
     *
     * @return void
     */
    public function initialize()
    {
        $this->registerMethod('getList');
        $this->registerMethod('get');
        $this->registerMethod('set');
        $this->registerMethod('delete');
    }

    /**
     * Get the list of pool configurations.
     *
     * @param array $params
     * @param array $context
     *
     * @return array
     */
    public function getList($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.common.getlist');

        $objects = $this->database->get($this->dataModelPath);
        $pools = [];

        foreach ($objects as $object) {
            $pools[] = $object->getAssoc();
        }

        return $this->applyFilter(
            $pools,
            $params['start'],
            $params['limit'],
            $params['sortfield'],
            $params['sortdir']
        );
    }

    /**
     * Get a single pool configuration by its UUID.
     *
     * @param array $params
     * @param array $context
     *
     * @return array
     */
    public function get($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.common.objectuuid');

        $object = $this->database->get($this->dataModelPath, $params['uuid']);

        return $object->getAssoc();
    }

    /**
     * Set a single pool configuration and return the new configuration.
     *
     * @param array $params
     * @param array $context
     *
     * @return array
     */
    public function set($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.phpfpm.set');

        $object = new ConfigObject($this->dataModelPath);
        $object->setAssoc($params);
        $isNewObject = $object->isNew();

        if ($isNewObject) {
            $this->database->assertIsUnique($object, 'name');
        }

        $this->database->set($object);

        return $object->getAssoc();
    }

    /**
     * Delete a pool configuration by its UUID.
     *
     * @param array $params
     * @param array $context
     *
     * @return array
     */
    public function delete($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.common.objectuuid');

        $object = $this->database->get($this->dataModelPath, $params['uuid']);
        $this->database->assertIsNotReferenced($object);

        $this->database->delete($object);
    }
}
