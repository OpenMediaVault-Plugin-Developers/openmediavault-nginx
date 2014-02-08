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
    const HOST_TYPE_PORT = "port";
    const HOST_TYPE_NAME = "name";

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
        return "Nginx";
    }

    public function initialize()
    {
        $this->registerMethod("getSettings");
        $this->registerMethod("setSettings");
        $this->registerMethod("getList");

        // CRUD
        $this->registerMethod("get");
        $this->registerMethod("set");
        $this->registerMethod("delete");
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
        $settingsObject["enable"]                        = boolval($object["enable"]);
        $settingsObject["server_names_hash_bucket_size"] = intval($object["server_names_hash_bucket_size"]);

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
                "enable"                        : { "type" : "boolean" },
                "server_names_hash_bucket_size" : { "type" : "integer", "enum" : [ 32, 64, 128 ] }
            }
        }');

        // Prepare configuration data
        $object = array(
            "enable"                        => array_boolval($params, "enable"),
            "server_names_hash_bucket_size" => $params["server_names_hash_bucket_size"]
        );

        if ($this->portIsUsed($object["server_name_default_port"], null, true))
            throw new OMVException(OMVErrorMsg::E_CONFIG_OBJECT_INVALID,
                "Selected port is already used");

        // Get old configuration object
        $oldConfig = $xmlConfig->get($this->getXPath());
        if (is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath());

        $object["servers"] = $oldConfig["servers"];

        // Set configuration object
        if (false === $xmlConfig->replace($this->getXPath(), $object))
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
                $servers[] = $this->get(array("uuid" => $uuid), $context);
            } catch (Exception $e) {
                throw $e;
            }
        }

        return $this->applyFilter(
            $servers,
            $params["start"],
            $params["count"],
            $params["sortfield"],
            $params["sortdir"]
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

        $server = $xmlConfig->get($this->getServerXPathByUuid($params["uuid"]));

        if (is_null($server))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $xPath);

        // General
        $server["enable"]                       = boolval($server["enable"]);
        $server["use_public_directory"]         = boolval($server["use_public_directory"]);
        $server["port"]                         = intval($server["port"]);
        
        // SSL
        $server["ssl_enable"]        = boolval($server["ssl_enable"]);
        $server["ssl_port"]          = intval($server["ssl_port"]);
        $server["ssl_force"]         = boolval($server["ssl_force"]);

        // PHP
        $server["php_enable"]              = boolval($server["php_enable"]);
        $server["php_display_errors"]      = boolval($server["php_display_errors"]);
        $server["php_html_errors"]         = boolval($server["php_html_errors"]);
        $server["php_max_execution_time"]  = intval($server["php_max_execution_time"]);
        $server["php_memory_limit"]        = intval($server["php_memory_limit"]);
        $server["php_post_max_size"]       = intval($server["php_post_max_size"]);
        $server["php_upload_max_filesize"] = intval($server["php_upload_max_filesize"]);

        // Options
        $server["autoindex"]                   = boolval($server["autoindex"]);
        $server["deny_htaccess"]               = boolval($server["deny_htaccess"]);
        $server["large_client_header_buffers"] = intval($server["large_client_header_buffers"]);

        // Extra options
        $server["extra_options"] = $server["extra_options"];

        // If ports is missing a value, just set default value
        if (!$server["port"])
            $server["port"] = 80;
        if (!$server["ssl_port"])
            $server["ssl_port"] = 443;

        // Get full path for presentation
        $xPath = sprintf("//system/shares/sharedfolder[uuid='%s']", $server["sharedfolderref"]);
        $sharedFolder = $xmlConfig->get($xPath);

        $xPath = sprintf("//system/fstab/mntent[uuid='%s']", $sharedFolder["mntentref"]);
        $mountPoint = $xmlConfig->get($xPath);

        $server["root_full_path"] = sprintf("%s/%s",
            $mountPoint["dir"],
            $sharedFolder["reldirpath"]);

        if ($server["use_public_directory"])
            $server["root_full_path"] .= "/" . $server["public_directory"];

        // Build possible URLs
        $urls = array();

        if ($server["host_type"] == self::HOST_TYPE_PORT) {
            if (!$server["ssl_force"])
                $urls[] = $this->buildHref("!domain!", $server["port"]);
            
            if ($server["ssl_enable"])
                $urls[] = $this->buildHref("!domain!", $server["ssl_port"], true);
        }
        
        if ($server["host_type"] == self::HOST_TYPE_NAME) {
            if (!$server["ssl_force"])
                $urls[] = $this->buildHref($server["server_name"], $server["port"]);
            
            if ($server["ssl_enable"])
                $urls[] = $this->buildHref($server["server_name"], $server["ssl_port"], true);
        }
        
        $server["urls"] = $urls;

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
                "host_type"                    : { "type" : "string", "enum" : [ "port", "name" ] },
                "server_name"                  : { "type" : "string" },
                "port"                         : { "type" : "integer" },
                
                "ssl_enable"        : { "type" : "boolean" },
                "ssl_port"          : { "type" : "integer" },
                "sslcertificateref" : {' . $GLOBALS['OMV_JSONSCHEMA_UUID_NONE'] . '},
                "ssl_force"         : { "type" : "boolean" },

                "php_enable"              : { "type" : "boolean" },
                "php_user"                : { "type" : "string" },
                "php_display_errors"      : { "type" : "boolean" },
                "php_html_errors"         : { "type" : "boolean" },
                "php_max_execution_time"  : { "type" : "integer", "minimum" : 0 },
                "php_memory_limit"        : { "type" : "integer", "minimum" : -1 },
                "php_post_max_size"       : { "type" : "integer", "minimum" : 1 },
                "php_upload_max_filesize" : { "type" : "integer", "minimum" : 1 },

                "autoindex"                   : { "type" : "boolean" },
                "deny_htaccess"               : { "type" : "boolean" },
                "large_client_header_buffers" : { "type" : "integer", "enum" : [ 8, 16, 32 ]},

                "extra_options" : { "type" : "string" }
            }
        }');

        $server = array(
            "uuid"                         => empty($params["uuid"]) ? OMVUtil::uuid() : $params["uuid"],
            "enable"                       => boolval($params["enable"]),
            "sharedfolderref"              => $params["sharedfolderref"],
            "use_public_directory"         => boolval($params["use_public_directory"]),
            "public_directory"             => $params["public_directory"],
            "host_type"                    => $params["host_type"],
            "server_name"                  => $params["server_name"],
            "port"                         => intval($params["port"]),
            
            "ssl_enable"        => boolval($params["ssl_enable"]),
            "ssl_port"          => intval($params["ssl_port"]),
            "sslcertificateref" => $params["sslcertificateref"],
            "ssl_force"         => boolval($params["ssl_force"]),

            "php_enable"              => boolval($params["php_enable"]),
            "php_user"                => $params["php_user"],
            "php_display_errors"      => boolval($params["php_display_errors"]),
            "php_html_errors"         => boolval($params["php_html_errors"]),
            "php_memory_limit"        => intval($params["php_memory_limit"]),
            "php_max_execution_time"  => intval($params["php_max_execution_time"]),
            "php_post_max_size"       => intval($params["php_post_max_size"]),
            "php_upload_max_filesize" => intval($params["php_upload_max_filesize"]),

            "autoindex"                   => boolval($params["autoindex"]),
            "deny_htaccess"               => boolval($params["deny_htaccess"]),
            "large_client_header_buffers" => intval($params["large_client_header_buffers"]),

            "extra_options" => $params["extra_options"]
        );

        // Remove current port number if not used
        if ($server["ssl_force"])
            $server["port"] = null;
        if (!$server["ssl_enable"])
            $server["ssl_port"] = null;

        // Perform port checks
        if ($server["port"] == $server["ssl_port"])
            throw new OMVException(OMVErrorMsg::E_CONFIG_OBJECT_INVALID,
                "Ports identical, can't use same port for SSL.");
                
        if (!$this->validatePort($server["port"], $server["uuid"], $server["host_type"]))
            throw new OMVException(OMVErrorMsg::E_CONFIG_OBJECT_INVALID,
                "Selected port is already used");
                
        if (!$this->validatePort($server["ssl_port"], $server["uuid"], $server["host_type"], true))
            throw new OMVException(OMVErrorMsg::E_CONFIG_OBJECT_INVALID,
                "Selected port (SSL) is already used");

        // Save configuration
        $success = false;
        $xPath = null;

        if (empty($params["uuid"]))
            $success = $xmlConfig->set($xPath = $this->getServersXPath(),
                array("server" => $server),
                $server);
        else
            $success = $xmlConfig->replace($xPath = $this->getServerXPathByUuid($params["uuid"]),
                $server);

        if (!$success)
            throw new OMVException(OMVErrorMsg::E_CONFIG_OBJECT_NOT_FOUND,
                $xPath);

        // Notify configuration changes
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(empty($params["uuid"]) ? OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
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

        $server = $xmlConfig->delete($this->getServerXPathByUuid($params["uuid"]));

        if (is_null($server))
            throw new OMVException(OMVErrorMsg::E_CONFIG_OBJECT_NOT_FOUND,
                $this->getServerXPathByUuid($params["uuid"]));

        // Notify configuration changes
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(OMV_NOTIFY_DELETE,
            "org.openmediavault.services.nginx", $server);
    }

    private function validatePort($port, $uuid, $hostType, $sslPort = false)
    {
        global $xmlConfig;

        $xPath = sprintf("%s/server[port='%s' and ssl_force='0']", $this->getServersXPath(), $port);
        $server = $xmlConfig->get($xPath);
        
        $sslXPath = sprintf("%s/server[ssl_port='%s' and ssl_enable='1']", $this->getServersXPath(), $port);
        $sslServer = $xmlConfig->get($sslXPath);

        // If not empty and not the same as 
        // the current one, check if port types
        // mismatches and then if any of them
        // are port based
        if (!empty($server)) {
            if ($server["uuid"] != $uuid) {
                if ($sslPort)
                    return false;
                if ($hostType == self::HOST_TYPE_PORT || $server["host_type"] == self::HOST_TYPE_PORT)
                    return false;
            } else {
                if ($sslPort)
                    return false;
            }
        }
        
        if (!empty($sslServer)) {
            if ($sslServer["uuid"] != $uuid) {
                if (!$sslPort)
                    return false;
                if ($hostType == self::HOST_TYPE_PORT || $server["host_type"] == self::HOST_TYPE_PORT)
                    return false;
            } else {
                if (!$sslPort)
                    return false;
            }
        }

        // Validate with webadmin ports
        $webadminConfig = $xmlConfig->get("//webadmin");

        if (!empty($webadminConfig)) {
            $webadminPort    = intval($webadminConfig["port"]);
            $webadminSslPort = intval($webadminConfig["sslport"]);
            $forceSslOnly    = boolval($webadminConfig["forcesslonly"]);
            $enableSsl       = boolval($webadminConfig["enablessl"]);

            if ($webadminPort == $port && !$forceSslOnly)
                return false;
            if ($webadminSslPort == $port && $enableSsl)
                return false;
        }

        return true;
    }

    private function buildHref($domain, $port, $ssl = false)
    {
        $url = sprintf("http%s://%s:%s", $ssl ? "s" : "", $domain, $port);
        return "<a href='$url' target='_blank'>$url</a>";
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceNginx());
