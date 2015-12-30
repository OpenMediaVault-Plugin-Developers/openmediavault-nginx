<?php

/**
 * Copyright (C) 2014-2015 OpenMediaVault Plugin Developers
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
require_once "openmediavault/rpcservice.inc";
require_once "openmediavault/util.inc";

class OMVRpcServiceNginxServer extends OMVRpcServiceAbstract
{
    const HOST_TYPE_PORT = "port";
    const HOST_TYPE_NAME = "name";

    private $logDir = "/var/log/nginx";

    private function getEventMessagePath()
    {
        return "org.openmediavault.services.nginx";
    }

    private function getPhpFpmEventMessagePath()
    {
        return "org.openmediavault.services.phpfpm";
    }

    private function getXpath()
    {
        return "/config/services/nginx";
    }

    private function getServersXpath()
    {
        return sprintf("%s/servers", $this->getXpath());
    }

    private function getServerXpathByUuid($uuid)
    {
        return sprintf("%s/server[uuid='%s']", $this->getServersXpath(), $uuid);
    }

    /**
     * Get the name of the RPC service.
     *
     * @return string
     */
    public function getName()
    {
        return "Nginx";
    }

    /**
     * Initialize the RPC service.
     *
     * @return void
     */
    public function initialize()
    {
        $this->registerMethod("getSettings");
        $this->registerMethod("setSettings");
        $this->registerMethod("getList");

        // CRUD
        $this->registerMethod("get");
        $this->registerMethod("set");
        $this->registerMethod("delete");

        // Log
        $this->registerMethod("getLog");
    }

    /**
     * Get main settings.
     *
     * @param array $params
     * @param array $context
     *
     * @return array
     */
    public function getSettings($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR
        ]);

        // Get configuration object
        $object = $xmlConfig->get($this->getXpath());
        if (is_null($object)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $this->getXpath());
        }

        // Modify result data.
        $settingsObject["enable"] = boolval($object["enable"]);
        $settingsObject["server_names_hash_bucket_size"] = intval($object["server_names_hash_bucket_size"]);

        return $settingsObject;
    }

    /**
     * Set main settings.
     *
     * @param array $params
     * @param array $context
     *
     * @return array
     */
    public function setSettings($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR
        ]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type": "object",
                "properties": {
                    "enable": { "type": "boolean" },
                    "server_names_hash_bucket_size": { "type": "integer", "enum": [32, 64, 128] }
                }
            }'
        );

        // Prepare configuration data.
        $object = [
            "enable" => array_boolval($params, "enable"),
            "server_names_hash_bucket_size" => $params["server_names_hash_bucket_size"]
        ];

        // Get old configuration object.
        $oldConfig = $xmlConfig->get($this->getXpath());
        if (is_null($object)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $this->getXpath());
        }

        $object["servers"] = $oldConfig["servers"];

        // Set configuration object.
        if (false === $xmlConfig->replace($this->getXpath(), $object)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED, $this->$xpath);
        }

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(
            OMV_NOTIFY_MODIFY,
            $this->getEventMessagePath(),
            $object
        );
        $dispatcher->notify(
            OMV_NOTIFY_MODIFY,
            $this->getPhpFpmEventMessagePath()
        );

        return $object;
    }

    /**
     * Get the list of server configurations.
     *
     * @param array $params
     * @param array $context
     *
     * @return array
     */
    public function getList($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR
        ]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type": "object",
                "properties": {
                    "start": { "type": "integer" },
                    "limit": {' . $GLOBALS["OMV_JSONSCHEMA_COUNTFIELD"] . '},
                    "sortfield": {' . $GLOBALS["OMV_JSONSCHEMA_SORTFIELD"] . '},
                    "sortdir": {' . $GLOBALS["OMV_JSONSCHEMA_SORTDIR"] . '}
                }
            }'
        );

        // Get configuration object.
        $xpath = sprintf("%s/server/uuid", $this->getServersXpath());
        $uuids = $xmlConfig->getList($xpath);

        if (is_null($uuids)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);
        }

        $servers = [];

        foreach ($uuids as $uuid) {
            try {
                $servers[] = $this->get(["uuid" => $uuid], $context);
            } catch (Exception $e) {
                throw $e;
            }
        }

        return $this->applyFilter(
            $servers,
            $params["start"],
            $params["limit"],
            $params["sortfield"],
            $params["sortdir"]
        );
    }

    /**
     * Get a single server configuration by its UUID.
     *
     * @param array $params
     * @param array $context
     *
     * @return array
     */
    public function get($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR
        ]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type": "object",
                "properties": {
                    "uuid": {' . $GLOBALS["OMV_JSONSCHEMA_UUID_NONE"] . '}
                }
            }'
        );

        $server = $xmlConfig->get($this->getServerXpathByUuid($params["uuid"]));

        if (is_null($server)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getServerXpathByUuid($params["uuid"])
            );
        }

        // General.
        $server["enable"] = boolval($server["enable"]);
        $server["use_root"] = boolval($server["use_root"]);
        $server["use_public_directory"] = boolval($server["use_public_directory"]);
        $server["port"] = intval($server["port"]);
        $server["port_default_server"] = boolval($server["port_default_server"]);

        // SSL.
        $server["ssl_enable"] = boolval($server["ssl_enable"]);
        $server["ssl_port"] = intval($server["ssl_port"]);
        $server["ssl_port_default_server"] = boolval($server["ssl_port_default_server"]);
        $server["ssl_force"] = boolval($server["ssl_force"]);

        // PHP.
        $server["php_enable"] = boolval($server["php_enable"]);
        $server["php_use_default_config"] = boolval($server["php_use_default_config"]);

        // Options.
        $server["use_index"] = boolval($server["use_index"]);
        $server["use_index_html"] = boolval($server["use_index_html"]);
        $server["use_index_php"] = boolval($server["use_index_php"]);
        $server["autoindex"] = boolval($server["autoindex"]);
        $server["deny_htaccess"] = boolval($server["deny_htaccess"]);
        $server["log_enable"] = boolval($server["log_enable"]);
        $server["large_client_header_buffers"] = intval($server["large_client_header_buffers"]);

        // If ports is missing a value, just set default value.
        if (!$server["port"]) {
            $server["port"] = 80;
        }

        if (!$server["ssl_port"]) {
            $server["ssl_port"] = 443;
        }

        // Get full path for presentation.
        $rootFullPath = null;

        if (!empty($server["sharedfolderref"])) {
            $rootFullPath = $this->getSharedFolderPathByUuid(
                $server["sharedfolderref"]
            );

            if ($server["use_root"] && $server["use_public_directory"]) {
                $rootFullPath = build_path([
                    $rootFullPath,
                    $server["public_directory"]
                ]);
            }
        }

        $server["root_full_path"] = $rootFullPath;

        // Build possible URLs.
        $urls = [];
        $serverNames = [];

        if ($server["host_type"] == self::HOST_TYPE_PORT) {
            $serverNames[] = "!domain!";
        } else {
            $serverNames = explode(" ", $server["server_name"]);
        }

        foreach ($serverNames as $domain) {
            if (!$server["ssl_force"]) {
                $urls[] = sprintf("http://%s:%s", $domain, $server["port"]);
            }

            if ($server["ssl_enable"]) {
                $urls[] = sprintf("https://%s:%s", $domain, $server["ssl_port"]);
            }
        }

        $server["urls"] = $urls;

        return $server;
    }

    /**
     * Set a single server configuration and return the new configuration.
     *
     * @param array $params
     * @param array $context
     *
     * @return array
     */
    public function set($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR
        ]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type": "object",
                "properties": {
                    "uuid": {' . $GLOBALS["OMV_JSONSCHEMA_UUID_UNDEFINED"] . '},
                    "enable": { "type": "boolean" },
                    "sharedfolderref": {' . $GLOBALS["OMV_JSONSCHEMA_UUID_NONE"] . '},
                    "use_root": { "type": "boolean" },
                    "use_public_directory": { "type": "boolean" },
                    "public_directory": { "type": "string", "optional": true },
                    "host_type": { "type": "string", "enum": ["port", "name"] },
                    "server_name": { "type": "string" },
                    "port": { "type": "integer" },
                    "port_default_server": { "type": "boolean" },

                    "ssl_enable": { "type": "boolean" },
                    "ssl_port": { "type": "integer" },
                    "ssl_port_default_server": { "type": "boolean" },
                    "sslcertificateref": {' . $GLOBALS["OMV_JSONSCHEMA_UUID_NONE"] . '},
                    "ssl_force": { "type": "boolean" },

                    "php_enable": { "type": "boolean" },
                    "php_pool_ref": {' . $GLOBALS["OMV_JSONSCHEMA_UUID_NONE"] . '},
                    "php_use_default_config": { "type": "boolean" },

                    "use_index": { "type": "boolean" },
                    "use_index_html": { "type": "boolean" },
                    "use_index_php": { "type": "boolean" },
                    "autoindex": { "type": "boolean" },
                    "deny_htaccess": { "type": "boolean" },
                    "log_enable": { "type" : "boolean" },
                    "large_client_header_buffers": { "type": "integer", "enum": [8, 16, 32]},

                    "extra_options": { "type": "string" }
                }
            }'
        );

        $server = [
            "uuid" =>
                ($params["uuid"] == $GLOBALS["OMV_UUID_UNDEFINED"]) ?
                OMVUtil::uuid() : $params["uuid"],
            "enable" => boolval($params["enable"]),
            "sharedfolderref" => $params["sharedfolderref"],
            "use_root" => boolval($params["use_root"]),
            "use_public_directory" => boolval($params["use_public_directory"]),
            "public_directory" => $params["public_directory"],
            "host_type" => $params["host_type"],
            "server_name" => $params["server_name"],
            "port" => intval($params["port"]),
            "port_default_server" => boolval($params["port_default_server"]),

            "ssl_enable" => boolval($params["ssl_enable"]),
            "ssl_port" => intval($params["ssl_port"]),
            "ssl_port_default_server" => boolval($params["ssl_port_default_server"]),
            "sslcertificateref" => $params["sslcertificateref"],
            "ssl_force" => boolval($params["ssl_force"]),

            "php_enable" => boolval($params["php_enable"]),
            "php_pool_ref" => $params["php_pool_ref"],
            "php_use_default_config" => boolval($params["php_use_default_config"]),

            "use_index" => boolval($params["use_index"]),
            "use_index_html" => boolval($params["use_index_html"]),
            "use_index_php" => boolval($params["use_index_php"]),
            "autoindex" => boolval($params["autoindex"]),
            "deny_htaccess" => boolval($params["deny_htaccess"]),
            "log_enable" => boolval($params["log_enable"]),
            "large_client_header_buffers" => intval($params["large_client_header_buffers"]),

            "extra_options" => $params["extra_options"],
        ];

        $server = $this->sanitizeServer($server);
        $this->validateServer($server);

        // Save configuration.
        $success = false;
        $xpath = null;

        if ($params["uuid"] == $GLOBALS["OMV_UUID_UNDEFINED"]) {
            $success = $xmlConfig->set(
                $xpath = $this->getServersXpath(),
                ["server" => $server],
                $server
            );
        } else {
            $success = $xmlConfig->replace(
                $xpath = $this->getServerXpathByUuid($params["uuid"]),
                $server
            );
        }

        if (!$success) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_OBJECT_NOT_FOUND, $xpath);
        }

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(
            $params["uuid"] == $GLOBALS["OMV_UUID_UNDEFINED"] ?
            OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
            $this->getEventMessagePath(),
            $server
        );

        return $server;
    }

    /**
     * Delete a server configuration by its UUID.
     *
     * @param array $params
     * @param array $context
     *
     * @return void
     */
    public function delete($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR
        ]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type": "object",
                "properties": {
                    "uuid": {' . $GLOBALS["OMV_JSONSCHEMA_UUID_NONE"] . '}
                }
            }'
        );

        $server = $xmlConfig->delete($this->getServerXpathByUuid($params["uuid"]));

        if (is_null($server)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_OBJECT_NOT_FOUND,
                $this->getServerXpathByUuid($params["uuid"])
            );
        }

        // Notify configuration changes
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(OMV_NOTIFY_DELETE, $this->getEventMessagePath(), $server);
    }

    /**
     * Get a Nginx server log. Returns either the access or error-log in array
     * format.
     *
     * @param array $params
     * @param array $context
     *
     * @return array
     */
    public function getLog($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR
        ]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type": "object",
                "properties": {
                    "start": { "type": "integer" },
                    "limit": {' . $GLOBALS["OMV_JSONSCHEMA_COUNTFIELD"] . '},
                    "sortfield": {' . $GLOBALS["OMV_JSONSCHEMA_SORTFIELD"] . '},
                    "sortdir": {' . $GLOBALS["OMV_JSONSCHEMA_SORTDIR"] . '},
                    "uuid": {' . $GLOBALS["OMV_JSONSCHEMA_UUID"] . '},
                    "type": { "type": "string", "enum": ["access", "error"]}
                }
            }'
        );

        $uuid = $params["uuid"];
        $type = $params["type"];

        $logRows = [];

        $logPath = sprintf(
            "%s/%s-%s.log",
            $this->logDir,
            $uuid,
            $type
        );

        if (file_exists($logPath)) {
            $logText = file_get_contents($logPath);

            $logTextArray = explode("\n", $logText);
            $rowNumber = 1;

            foreach ($logTextArray as $row) {
                if (!empty($row)) {
                    $logRows[] = [
                        "id" => $rowNumber++,
                        "message" => $row,
                    ];
                }
            }
        }

        return [
            "total" => count($logRows),
            "data" => $logRows,
        ];
    }

    /**
     * Sanitize a server configuration. Corrects different configuration
     * options.
     *
     * @param array $server The configuration to sanitize.
     *
     * @return array Returns the sanitized server configuration.
     */
    private function sanitizeServer($server)
    {
        // Remove port numbers that aren't used.
        if ($server["ssl_force"]) {
            $server["port"] = null;
        }

        if (!$server["ssl_enable"]) {
            $server["ssl_port"] = null;
        }

        // Reset the default_server option to false when the port isn't used.
        if ($server["ssl_force"]) {
            $server["port_default_server"] = false;
        }

        if (!$server["ssl_enable"]) {
            $server["ssl_port_default_server"] = false;
        }

        // Trim whitespace and then trim slashes at start and end from the
        // public directory.
        $server["public_directory"] = trim($server["public_directory"]);
        $server["public_directory"] = trim($server["public_directory"], DIRECTORY_SEPARATOR);

        // Trim and replace all spaces in the server_name with a single space to
        // allow for easy exploding of the string.
        $serverName = $server["server_name"];
        $serverName = trim($serverName);
        $serverName = preg_replace("/\s+/", " ", $serverName);
        $server["server_name"] = $serverName;

        // Trim whitespace from extra options.
        $server["extra_options"] = trim($server["extra_options"]);

        return $server;
    }

    /**
     * Validate server options. Throws exceptions when errors occurs.
     *
     * @param array $server The server configuration to validate.
     *
     * @throws Exception
     *
     * @return void
     */
    private function validateServer($server)
    {
        // Perform port checks.
        if ($server["port"] == $server["ssl_port"]) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_OBJECT_INVALID,
                "Ports identical, can't use same port for SSL."
            );
        }

        if (!$this->validatePort($server["port"], $server["uuid"], $server["host_type"])) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_OBJECT_INVALID,
                "The selected port is already in use."
            );
        }

        if (!$this->validatePort($server["ssl_port"], $server["uuid"], $server["host_type"], true)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_OBJECT_INVALID,
                "The selected SSL port is already in use."
            );
        }

        // Validate the default server attribute.
        if (!$server["ssl_force"]) {
            if ($server["port_default_server"]) {
                if (!$this->validatePortDefaultServer($server["port"], $server["uuid"])) {
                    throw new OMVException(
                        OMVErrorMsg::E_CONFIG_OBJECT_INVALID,
                        "There is already another server set as the default on the selected port."
                    );
                }
            }
        }

        if ($server["ssl_enable"]) {
            if ($server["ssl_port_default_server"]) {
                if (!$this->validatePortDefaultServer($server["ssl_port"], $server["uuid"], true)) {
                    throw new OMVException(
                        OMVErrorMsg::E_CONFIG_OBJECT_INVALID,
                        "There is already another server set as the default on the selected SSL port."
                    );
                }
            }
        }
    }

    /**
     * Validate port usage.
     *
     * Using the same ports as the web interface is only allowed with a
     * name-based server with matching SSL setting (on/off).
     *
     * A port can only be used multiple times when it's used in a name-based
     * server.
     *
     * Mixing SSL servers with non SSL servers is not allowed and will also fail
     * the validation.
     *
     * @param int $port The port to check.
     * @param string $uuid ID of server to check.
     * @param string $hostType Indicate if the port is used in a port or
     * name-based setup.
     * @param bool $sslPort Indicate wether the port is used as an SSL port.
     *
     * @return bool
     */
    private function validatePort($port, $uuid, $hostType, $sslPort = false)
    {
        global $xmlConfig;

        // Validate against web interface ports.
        $webadminConfig = $xmlConfig->get("/config/webadmin");

        if (!empty($webadminConfig)) {
            $webadminPort = intval($webadminConfig["port"]);
            $webadminSslPort = intval($webadminConfig["sslport"]);
            $enableSsl = boolval($webadminConfig["enablessl"]);

            // We don't need to check if force SSL only is enabled since the web
            // interface configuration still uses the http port to redirect to
            // https.
            //
            // If the host type is name-based we will still allow using the web
            // interface ports.
            if ($webadminPort == $port && ($hostType == self::HOST_TYPE_PORT || $sslPort)) {
                return false;
            }

            if ($webadminSslPort == $port && $enableSsl && ($hostType == self::HOST_TYPE_PORT || !$sslPort)) {
                return false;
            }
        }

        // Get the server configurations matching the port to validage against.
        // When SSL is forced the non SSL port isn't used, so there's no need to
        // validate those entries.
        $xpath = sprintf("%s/server[port='%s' and ssl_force='0']", $this->getServersXpath(), $port);
        $server = $xmlConfig->get($xpath);

        $sslXPath = sprintf("%s/server[ssl_port='%s' and ssl_enable='1']", $this->getServersXpath(), $port);
        $sslServer = $xmlConfig->get($sslXPath);

        if (!empty($server)) {
            // SSL and non SSL port mismatch.
            if ($sslPort) {
                return false;
            }

            if ($server["uuid"] != $uuid) {
                if ($hostType == self::HOST_TYPE_PORT || $server["host_type"] == self::HOST_TYPE_PORT) {
                    return false;
                }
            }
        }

        if (!empty($sslServer)) {
            // Non SSL and SSL port mismatch.
            if (!$sslPort) {
                return false;
            }

            if ($sslServer["uuid"] != $uuid) {
                if ($hostType == self::HOST_TYPE_PORT || $server["host_type"] == self::HOST_TYPE_PORT) {
                    return false;
                }
            }
        }

        return true;
    }

    /**
     * Validate the default_server option.
     *
     * If the port is used by the web interface the validation will fail since
     * the web interface always has the default_server option set.
     *
     * The validation succeeds if the selected port doesn't have the
     * default_server option set in any existing server configuration. The
     * validation will also succeed if the default_server option is set on the
     * server configuration provided by ID reference.
     *
     * @param int $port The port number that should be checked.
     * @param string $uuid ID of the server configuration to be checked.
     * @param bool $ssl Indicate wether to validate against an SSL enabled
     * server or not. Defaults to false.
     *
     * @return bool
     */
    private function validatePortDefaultServer($port, $uuid, $ssl = false)
    {
        global $xmlConfig;

        $webadminConfig = $xmlConfig->get("//webadmin");

        if ($webadminConfig["port"] == $port) {
            return false;
        }

        if ($webadminConfig["sslport"] == $port) {
            return false;
        }

        $xpath = sprintf(
            "%s/server[port='%s' and port_default_server='1' and uuid!='%s']",
            $this->getServersXpath(),
            $port,
            $uuid
        );

        if ($ssl) {
            $xpath = sprintf(
                "%s/server[ssl_port='%s' and ssl_port_default_server='1' and uuid!='%s']",
                $this->getServersXpath(),
                $port,
                $uuid
            );
        }

        return $xmlConfig->count($xpath) === 0;
    }

    /**
     * Get path of a shared folder by its ID.
     *
     * @param string $uuid ID of the shared folder.
     *
     * @return string Path to shared folder.
     */
    private function getSharedFolderPathByUuid($uuid)
    {
        $path = OMVRpc::exec("ShareMgmt", "getPath", [
            "uuid" => $uuid,
        ], $this->getAdminContext());

        return $path;
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceNginxServer());
