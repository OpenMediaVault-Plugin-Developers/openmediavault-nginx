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

require_once "openmediavault/error.inc";
require_once "openmediavault/notify.inc";
require_once "openmediavault/object.inc";
require_once "openmediavault/rpc.inc";
require_once "openmediavault/util.inc";

class OMVRpcServicePhpFpmServer extends OMVRpcServiceAbstract
{
    private function getEventMessagePath()
    {
        return "org.openmediavault.services.phpfpm";
    }

    private function getXPath()
    {
        return "//services/phpfpm";
    }

    private function getPoolsXPath()
    {
        return sprintf("%s/pools", $this->getXPath(), $uuid);
    }

    private function getPoolXPathByUuid($uuid)
    {
        return sprintf("%s/pool[uuid='%s']", $this->getPoolsXPath(), $uuid);
    }

    public function getName()
    {
        return "PhpFpm";
    }

    public function initialize()
    {
        $this->registerMethod("getList");
        $this->registerMethod("get");
        $this->registerMethod("set");
        $this->registerMethod("delete");
    }

    public function getList($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type"       : "object",
                "properties" : {
                    "start"     : { "type" : "integer" },
                    "limit"     : {' . $GLOBALS["OMV_JSONSCHEMA_COUNTFIELD"] . '},
                    "sortfield" : {' . $GLOBALS["OMV_JSONSCHEMA_SORTFIELD"] . '},
                    "sortdir"   : {' . $GLOBALS["OMV_JSONSCHEMA_SORTDIR"] . '}
                }
            }'
        );

        // Get configuration object
        $xPath = sprintf("%s/pool/uuid", $this->getPoolsXPath());
        $uuids = $xmlConfig->getList($xPath);

        if (is_null($uuids)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xPath);
        }

        $pools = array();

        foreach ($uuids as $uuid) {
            try {
                $pools[] = $this->get(array("uuid" => $uuid), $context);
            } catch (Exception $e) {
                throw $e;
            }
        }

        return $this->applyFilter(
            $pools,
            $params["start"],
            $params["count"],
            $params["sortfield"],
            $params["sortdir"]
        );
    }

    public function get($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type"       : "object",
                "properties" : {
                    "uuid" : {' . $GLOBALS["OMV_JSONSCHEMA_UUID"] . '}
                }
            }'
        );

        $pool = $xmlConfig->get($this->getPoolXPathByUuid($params["uuid"]));

        if (is_null($pool)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getPoolXPathByUuid($params["uuid"])
            );
        }

        // General
        $pool["pm_max_children"]         = intval($pool["pm_max_children"]);
        $pool["pm_start_servers"]        = intval($pool["pm_start_servers"]);
        $pool["pm_min_spare_servers"]    = intval($pool["pm_min_spare_servers"]);
        $pool["pm_max_spare_servers"]    = intval($pool["pm_max_spare_servers"]);
        $pool["pm_max_requests"]         = intval($pool["pm_max_requests"]);
        $pool["php_display_errors"]      = boolval($pool["php_display_errors"]);
        $pool["php_html_errors"]         = boolval($pool["php_html_errors"]);
        $pool["php_max_execution_time"]  = intval($pool["php_max_execution_time"]);
        $pool["php_memory_limit"]        = intval($pool["php_memory_limit"]);
        $pool["php_post_max_size"]       = intval($pool["php_post_max_size"]);
        $pool["php_upload_max_filesize"] = intval($pool["php_upload_max_filesize"]);

        return $pool;
    }

    public function set($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type"       : "object",
                "properties" : {
                    "uuid"                    : {' . $GLOBALS["OMV_JSONSCHEMA_UUID_UNDEFINED"] . '},
                    "name"                    : { "type" : "string" },
                    "description"             : { "type" : "string" },
                    "user"                    : { "type" : "string" },
                    "group"                   : { "type" : "string" },
                    "pm_max_children"         : { "type" : "integer", "minimum" : 1 },
                    "pm_start_servers"        : { "type" : "integer", "minimum" : 1 },
                    "pm_min_spare_servers"    : { "type" : "integer", "minimum" : 1 },
                    "pm_max_spare_servers"    : { "type" : "integer", "minimum" : 1 },
                    "pm_max_requests"         : { "type" : "integer", "minimum" : 0 },
                    "php_display_errors"      : { "type" : "boolean" },
                    "php_html_errors"         : { "type" : "boolean" },
                    "php_max_execution_time"  : { "type" : "integer", "minimum" : 0 },
                    "php_memory_limit"        : { "type" : "integer", "minimum" : -1 },
                    "php_post_max_size"       : { "type" : "integer", "minimum" : 1 },
                    "php_upload_max_filesize" : { "type" : "integer", "minimum" : 1 },
                    "extra_options"           : { "type" : "string" }
                }
            }'
        );

        $pool = array(
            "uuid"                    =>
                ($params["uuid"] == $GLOBALS["OMV_UUID_UNDEFINED"])
                ? OMVUtil::uuid() : $params["uuid"],
            "name"                    => $params["name"],
            "description"             => $params["description"],
            "user"                    => $params["user"],
            "group"                   => $params["group"],
            "pm_max_children"         => intval($params["pm_max_children"]),
            "pm_start_servers"        => intval($params["pm_start_servers"]),
            "pm_min_spare_servers"    => intval($params["pm_min_spare_servers"]),
            "pm_max_spare_servers"    => intval($params["pm_max_spare_servers"]),
            "pm_max_requests"         => intval($params["pm_max_requests"]),
            "php_display_errors"      => boolval($params["php_display_errors"]),
            "php_html_errors"         => boolval($params["php_html_errors"]),
            "php_memory_limit"        => intval($params["php_memory_limit"]),
            "php_max_execution_time"  => intval($params["php_max_execution_time"]),
            "php_post_max_size"       => intval($params["php_post_max_size"]),
            "php_upload_max_filesize" => intval($params["php_upload_max_filesize"]),
            "extra_options"           => trim($params["extra_options"]),
        );

        // Save configuration
        $success = false;
        $xPath = null;

        if ($params["uuid"] == $GLOBALS["OMV_UUID_UNDEFINED"]) {
            if (!$this->validateNewPool($pool)) {
                throw new OMVException(
                    OMVErrorMsg::E_CONFIG_OBJECT_INVALID,
                    sprintf("%s is in use.", $pool["name"])
                );
            }

            $success = $xmlConfig->set(
                $xPath = $this->getPoolsXPath(),
                array("pool" => $pool),
                $pool
            );
        } else {
            $success = $xmlConfig->replace(
                $xPath = $this->getPoolXPathByUuid($params["uuid"]),
                $pool
            );
        }

        if (!$success) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_OBJECT_NOT_FOUND, $xPath);
        }

        // Notify configuration changes
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(
            empty($params["uuid"]) ? OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
            $this->getEventMessagePath(),
            $pool
        );

        return $pool;
    }

    public function delete($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type"       : "object",
                "properties" : {
                    "uuid" : {' . $GLOBALS["OMV_JSONSCHEMA_UUID_NONE"] . '}
                }
            }'
        );

        $pool = $xmlConfig->delete($this->getPoolXPathByUuid($params["uuid"]));

        if (is_null($pool)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_OBJECT_NOT_FOUND,
                $this->getPoolXPathByUuid($params["uuid"])
            );
        }

        // Notify configuration changes
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(
            OMV_NOTIFY_DELETE,
            $this->getEventMessagePath(),
            $pool
        );
    }

    /**
     * Validate the values of a new FPM pool. Throws exceptions depending
     * on the validation error.
     *
     * @param array @pool
     *
     * @return boolean
     */
    private function validateNewPool($pool)
    {
        $xPath = sprintf(
            "%s/pool[name='%s']",
            $this->getPoolsXPath(),
            $pool["name"]
        );

        if (true === $xmlConfig->exists($xPath)) {
            return false;
        }

        return true;
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServicePhpFpmServer());
