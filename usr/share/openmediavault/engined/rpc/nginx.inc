<?php

/**
 * Copyright (C) 2014 OpenMediaVault Plugin Developers
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

require_once "openmediavault/object.inc";
require_once "openmediavault/error.inc";
require_once "openmediavault/util.inc";
require_once "openmediavault/rpc.inc";
require_once "openmediavault/notify.inc";

class OMVRpcServiceNginx extends OMVRpcServiceAbstract
{
    const HOST_TYPE_PORT = 0;
    const HOST_TYPE_NAME = 1;
    const HOST_TYPE_BOTH = 2;

    private $_defaultServerNamePort = null;

    private function getXPath()
    {
        return "//services/nginx";
    }

    private function getServersXPath()
    {
        return sprintf("%s/servers", $this->getXPath(), $uuid);
    }

    private function getServerXPathByUuid($uuid)
    {
        return sprintf("%s/server[uuid='%s']", $this->getServersXPath(), $uuid);
    }

    public function getName()
    {
        return 'Nginx';
    }

    public function initialize()
    {
        $this->registerMethod('getSettings');
        $this->registerMethod('setSettings');
        $this->registerMethod('getList');

        // CRUD
        $this->registerMethod('get');
        $this->registerMethod('set');
        $this->registerMethod('delete');

        // Validation
        $this->registerMethod('validatePort');
    }

    public function getSettings($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Get configuration object
        $object = $xmlConfig->get($this->getXPath());
        if (is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath());

        // Modify result data
        $settingsObject['enable']                   = boolval($object['enable']);
        $settingsObject['server_name_default_port'] = intval($object['server_name_default_port']);

        return $settingsObject;
    }

    public function setSettings($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "enable"                   : { "type" : "boolean" },
                "server_name_default_port" : { "type" : "integer" }
            }
        }');

        // Prepare configuration data
        $object = array(
            "enable"                   => array_boolval($params, 'enable'),
            "server_name_default_port" => $params['server_name_default_port']
        );

        if ($this->portIsUsed($object['server_name_default_port'], null, true))
            throw new OMVException(OMVErrorMsg::E_CONFIG_OBJECT_INVALID,
                "Selected port is already used");

        // Set configuration object
        $xPath = sprintf("%s/enable", $this->getXPath(), $object['enable']);
        if (false === $xmlConfig->replace($xPath, $object['enable']))
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED,
                $this->$xPath);

        $xPath = sprintf("%s/server_name_default_port", $this->getXPath());
        if (false === $xmlConfig->replace($xPath, $object['server_name_default_port']))
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED,
                $this->$xPath);

        // Notify configuration changes
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(OMV_NOTIFY_MODIFY, "org.openmediavault.services.nginx", $object);

        return $object;
    }

    function getList($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "start"     : { "type" : "integer" },
                "limit"     : {' . $GLOBALS['OMV_JSONSCHEMA_COUNTFIELD'] . '},
                "sortfield" : {' . $GLOBALS['OMV_JSONSCHEMA_SORTFIELD'] . '},
                "sortdir"   : {' . $GLOBALS['OMV_JSONSCHEMA_SORTDIR'] . '}
            }
        }');

        // Get configuration object
        $xPath = sprintf("%s/server/uuid", $this->getServersXPath());
        $uuids = $xmlConfig->getList($xPath);

        if (is_null($uuids))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $xPath);

        $servers = array();

        foreach ($uuids as $uuid) {
            try {
                $servers[] = $this->get(array('uuid' => $uuid), $context);
            } catch (Exception $e) {
                throw $e;
            }
        }

        return $this->applyFilter(
            $servers,
            $params['start'],
            $params['count'],
            $params['sortfield'],
            $params['sortdir']
        );
    }

    function get($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : {' . $GLOBALS['OMV_JSONSCHEMA_UUID_NONE'] . '}
            }
        }');

        $server = $xmlConfig->get($this->getServerXPathByUuid($params['uuid']));

        if (is_null($server))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $xPath);

        // General
        $server['enable']                       = boolval($server['enable']);
        $server['host_type']                    = intval($server['host_type']);
        $server['use_public_directory']         = boolval($server['use_public_directory']);
        $server['port']                         = intval($server['port']);
        $server['server_name_use_default_port'] = boolval($server['server_name_use_default_port']);

        // Php
        $server['php_enable'] = boolval($server['php_enable']);
        $server['php_user']   = $server['php_user'];

        // Options
        $server['autoindex']     = boolval($server['autoindex']);
        $server['deny_htaccess'] = boolval($server['deny_htaccess']);

        // Other options
        $server['other_options'] = $server['other_options'];

        // Get full path for presentation
        $xPath = sprintf("//system/shares/sharedfolder[uuid='%s']", $server['sharedfolderref']);
        $sharedFolder = $xmlConfig->get($xPath);

        $xPath = sprintf("//system/fstab/mntent[uuid='%s']", $sharedFolder['mntentref']);
        $mountPoint = $xmlConfig->get($xPath);

        $server['root_full_path'] = sprintf("%s/%s",
            $mountPoint['dir'],
            $sharedFolder['reldirpath']);

        if ($server['use_public_directory'])
            $server['root_full_path'] .= "/" . $server['public_directory'];

        // Build possible URLs
        $urls = array();

        // If host type is not only name, add port-based link
        if ($server['host_type'] != self::HOST_TYPE_NAME) {
            $urls[] = $this->buildHref('!domain!', $server['port']);
        }

        // If host type is name or both, add ServerName link
        if ($server['host_type'] > self::HOST_TYPE_PORT) {
            // Get correct port
            $port = $server['port'];

            if ($server['server_name_use_default_port'])
                $port = $this->getDefaultServerNamePort();

            $urls[] = $this->buildHref($server['server_name'], $port);
        }

        $server['urls'] = $urls;

        return $server;
    }

    public function set($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid"                         : {' . $GLOBALS['OMV_JSONSCHEMA_UUID_UNDEFINED'] . '},
                "enable"                       : { "type" : "boolean" },
                "sharedfolderref"              : {' . $GLOBALS['OMV_JSONSCHEMA_UUID'] . '},
                "use_public_directory"         : { "type" : "boolean" },
                "public_directory"             : { "type" : "string", "optional" : true },
                "host_type"                    : { "type" : "integer" },
                "port"                         : { "type" : "integer" },
                "server_name"                  : { "type" : "string" },
                "server_name_use_default_port" : { "type" : "boolean" },

                "php_enable" : { "type" : "boolean" },
                "php_user"   : { "type" : "string" },

                "autoindex"     : { "type" : "boolean" },
                "deny_htaccess" : { "type" : "boolean" },

                "extra_options" : { "type" : "string" }
            }
        }');

        $server = array(
            'uuid'                         => empty($params['uuid']) ? OMVUtil::uuid() : $params['uuid'],
            'enable'                       => boolval($params['enable']),
            'sharedfolderref'              => $params['sharedfolderref'],
            'use_public_directory'         => boolval($params['use_public_directory']),
            'public_directory'             => $params['public_directory'],
            'host_type'                    => intval($params['host_type']),
            'port'                         => intval($params['port']),
            'server_name'                  => $params['server_name'],
            'server_name_use_default_port' => boolval($params['server_name_use_default_port']),

            'php_enable' => boolval($params['php_enable']),
            'php_user'   => $params['php_user'],

            'autoindex'     => boolval($params['autoindex']),
            'deny_htaccess' => boolval($params['deny_htaccess']),

            'extra_options' => $params['extra_options']
        );

        // Check if choosen port is already used
        if ($server['host_type'] != self::HOST_TYPE_NAME ||
            $server['host_type'] == self::HOST_TYPE_NAME &&
            !$server['server_name_use_default_port'])
        {
            if ($this->portIsUsed($server['port'], $server['uuid']))
                throw new OMVException(OMVErrorMsg::E_CONFIG_OBJECT_INVALID,
                    "Selected port is already used");
        } else {
            unset($server['port']);
        }

        // Save configuration
        $success = false;
        $xPath = null;

        if (empty($params['uuid']))
            $success = $xmlConfig->set($xPath = $this->getServersXPath(),
                array('server' => $server),
                $server);
        else
            $success = $xmlConfig->replace($xPath = $this->getServerXPathByUuid($params['uuid']),
                $server);

        if (!$success)
            throw new OMVException(OMVErrorMsg::E_CONFIG_OBJECT_NOT_FOUND,
                $xPath);

        // Notify configuration changes
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(empty($params['uuid']) ? OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
            "org.openmediavault.services.nginx", $server);

        return $server;
    }

    public function delete($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : {' . $GLOBALS['OMV_JSONSCHEMA_UUID_NONE'] . '}
            }
        }');

        $server = $xmlConfig->delete($this->getServerXPathByUuid($params['uuid']));

        if (is_null($server))
            throw new OMVException(OMVErrorMsg::E_CONFIG_OBJECT_NOT_FOUND,
                $this->getServerXPathByUuid($params['uuid']));

        // Notify configuration changes
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(OMV_NOTIFY_DELETE,
            "org.openmediavault.services.nginx", $server);
    }

    public function validatePort($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : {' . $GLOBALS['OMV_JSONSCHEMA_UUID_NONE'] . '},
                "port" : { "type" : "integer" }
            }
        }');

        return !$this->portIsUsed($params['port'], $params['uuid']);
    }

    private function portIsUsed($port, $uuid, $isDefaultServerNamePort = false)
    {
        global $xmlConfig;

        $xPath = sprintf("%s/server[port='%s']", $this->getServersXPath(), $port);
        $server = $xmlConfig->get($xPath);

        if ($this->getDefaultServerNamePort() == $port && !$isDefaultServerNamePort)
            return true;
        if (!empty($server))
            if ($server['uuid'] != $uuid)
                return true;

        // Validate with webadmin ports
        $webadminConfig = $xmlConfig->get("//webadmin");

        if (!empty($webadminConfig)) {
            $webadminPort    = intval($webadminConfig['port']);
            $webadminSslPort = intval($webadminConfig['sslport']);
            $forceSslOnly    = boolval($webadminConfig['forcesslonly']);
            $enableSsl       = boolval($webadminConfig['enablessl']);

            if ($webadminPort == $port && !$forceSslOnly)
                return true;
            if ($webadminSslPort == $port && $enableSsl)
                return true;
        }

        return false;
    }

    private function buildHref($domain, $port)
    {
        $url = sprintf("http://%s:%s", $domain, $port);
        return "<a href='$url' target='_blank'>$url</a>";
    }

    private function getDefaultServerNamePort()
    {
        if (!$this->_defaultServerNamePort) {
            global $xmlConfig;
            $xPath = sprintf("%s/server_name_default_port", $this->getXPath());
            $this->_defaultServerNamePort = $xmlConfig->get($xPath);
        }

        return $this->_defaultServerNamePort;
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceNginx());
