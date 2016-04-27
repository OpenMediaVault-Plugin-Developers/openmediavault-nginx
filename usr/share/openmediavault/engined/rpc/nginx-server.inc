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
use OMV\Engine\Notify\Dispatcher;
use OMV\Exception;
use OMV\Rpc\Rpc;
use OMV\Rpc\ServiceAbstract;

class NginxServer extends ServiceAbstract
{
    const HOST_TYPE_PORT = 'port';
    const HOST_TYPE_NAME = 'name';

    /** @var Database */
    private $database;

    /** @var string */
    private $dataModelPath = 'conf.service.nginx';

    /** @var Dispatcher */
    private $dispatcher;

    /** @var string */
    private $eventMessagePath = 'org.openmediavault.conf.service.nginx';

    /** @var string */
    private $logDir = '/var/log/nginx';

    /** @var string */
    private $phpFpmEventMessagePath = 'org.openmediavault.conf.service.phpfpm';

    // FIXME: Validation is currently lacking uuidv4 because since have to allow
    //        the none value (empty string).
    /** @var string */
    private $serverDataModelPath = 'conf.service.nginx.server';

    public function __construct(Database $database = null, Dispatcher $dispatcher = null)
    {
        $this->database = $database ?: Database::getInstance();
        $this->dispatcher = $dispatcher ?: Dispatcher::getInstance();
    }

    /**
     * Get the name of the RPC service.
     *
     * @return string
     */
    public function getName()
    {
        return 'Nginx';
    }

    /**
     * Initialize the RPC service.
     *
     * @return void
     */
    public function initialize()
    {
        $this->registerMethod('getSettings');
        $this->registerMethod('setSettings');
        $this->registerMethod('getList');
        $this->registerMethod('get');
        $this->registerMethod('set');
        $this->registerMethod('delete');
        $this->registerMethod('getLog');
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
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        $object = $this->database->get($this->dataModelPath);
        $object->remove('servers');

        return $object->getAssoc();
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
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.nginx.setsettings');

        $object = $this->database->get($this->dataModelPath);
        $object->setAssoc($params);

        $this->database->set($object);
        $this->dispatcher->notify(OMV_NOTIFY_MODIFY, $this->eventMessagePath, $object);
        $this->dispatcher->notify(OMV_NOTIFY_MODIFY, $this->phpFpmEventMessagePath);

        $object->remove('servers');

        return $object->getAssoc();
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
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.common.getlist');

        $objects = $this->database->get($this->serverDataModelPath);
        $servers = [];

        foreach ($objects as $object) {
            // Get full path for presentation.
            $rootFullPath = '';

            if (!empty($object->get('sharedfolderref'))) {
                $rootFullPath = $this->getSharedFolderPathByUuid($object->get('sharedfolderref'));

                if ($object->get('use_root') && $object->get('use_public_directory')) {
                    $rootFullPath = build_path([$rootFullPath, $object->get('public_directory')]);
                }
            }

            $object->add('root_full_path', 'string');
            $object->set('root_full_path', $rootFullPath);

            // Build possible URLs.
            $urls = [];
            $serverNames = [];

            if ($object->get('host_type') === self::HOST_TYPE_PORT) {
                $serverNames[] = '!domain!';
            } else {
                $serverNames = explode(' ', $object->get('server_name'));
            }

            foreach ($serverNames as $domain) {
                if (!$object->get('ssl_force')) {
                    $urls[] = sprintf('http://%s:%s', $domain, $object->get('port'));
                }

                if ($object->get('ssl_enable')) {
                    $urls[] = sprintf('https://%s:%s', $domain, $object->get('ssl_port'));
                }
            }

            $object->add('urls', 'any');
            $object->set('urls', $urls);

            $servers[] = $object->getAssoc();
        }

        return $this->applyFilter(
            $servers,
            $params['start'],
            $params['limit'],
            $params['sortfield'],
            $params['sortdir']
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
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.common.objectuuid');

        $object = $this->database->get($this->serverDataModelPath, $params['uuid']);

        return $object->getAssoc();
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
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.nginx.set');

        $object = new ConfigObject($this->serverDataModelPath);
        $object->setAssoc($params);
        $isNewObject = $object->isNew();

        $object = $this->sanitizeServer($object);
        $this->validateServer($object);

        $this->database->set($object);

        $this->dispatcher->notify(
            $isNewObject ? OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
            $this->eventMessagePath,
            $object
        );

        return $object->getAssoc();
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
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.common.objectuuid');

        $object = $this->database->get($this->serverDataModelPath, $params['uuid']);

        $this->database->delete($object);
        $this->dispatcher->notify(OMV_NOTIFY_DELETE, $this->eventMessagePath, $object);
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
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.nginx.getlog');

        $uuid = $params['uuid'];
        $type = $params['type'];

        $logRows = [];

        $logPath = sprintf(
            '%s/%s-%s.log',
            $this->logDir,
            $uuid,
            $type
        );

        if (file_exists($logPath)) {
            $logText = file_get_contents($logPath);
            $logTextArray = explode(PHP_EOL, $logText);
            $rowNumber = 0;

            foreach ($logTextArray as $row) {
                if (!empty($row)) {
                    $logRows[] = [
                        'id' => ++$rowNumber,
                        'message' => $row,
                    ];
                }
            }
        }

        return [
            'total' => count($logRows),
            'data' => $logRows,
        ];
    }

    /**
     * Sanitize a server configuration. Corrects different configuration
     * options.
     *
     * @param ConfigObject $server The configuration to sanitize.
     *
     * @return ConfigObject Returns the sanitized server configuration.
     */
    private function sanitizeServer(ConfigObject $server)
    {
        // Reset the default_server option to false when the port isn't used.
        if ($server->get('ssl_force')) {
            $server->set('port_default_server', false);
        }

        if (!$server->get('ssl_enable')) {
            $server->set('ssl_port_default_server', false);
        }

        // Trim whitespace and then trim slashes at start and end from the
        // public directory.
        $server->set('public_directory', trim($server->get('public_directory')));
        $server->set('public_directory', trim($server->get('public_directory'), DIRECTORY_SEPARATOR));

        // Trim and replace all spaces in the server_name with a single space to
        // allow for easy exploding of the string.
        $serverName = $server->get('server_name');
        $serverName = trim($serverName);
        $serverName = preg_replace('/\s+/', ' ', $serverName);
        $server->set('server_name', $serverName);

        // Trim whitespace from extra options.
        $server->set('extra_options', trim($server->get('extra_options')));

        return $server;
    }

    /**
     * Validate server options. Throws exceptions when errors occurs.
     *
     * @param ConfigObject $server The server configuration to validate.
     *
     * @return void
     *
     * @throws Exception
     */
    private function validateServer(ConfigObject $server)
    {
        $sslEnable = $server->get('ssl_enable');
        $sslOnly = $server->get('ssl_force');
        $hostType = $server->get('host_type');
        $uuid = $server->get('uuid');

        $port = $sslOnly ? null : $server->get('port');
        $sslPort = $sslEnable ? $server->get('ssl_port') : null;

        // Perform port checks.
        if ($port === $sslPort) {
            throw new Exception('Ports identical, can\'t use the same port for SSL');
        }

        if (!$this->validatePort($port, $uuid, $hostType)) {
            throw new Exception('The selected port is already in use.');
        }

        if (!$this->validatePort($sslPort, $uuid, $hostType, true)) {
            throw new Exception('The selected SSL port is already in use.');
        }

        // Validate the default server attribute.
        if (!$sslOnly) {
            if ($server->get('port_default_server')) {
                if (!$this->validatePortDefaultServer($port, $uuid)) {
                    throw new Exception('There is already another server set as the default on the selected port.');
                }
            }
        }

        if ($sslEnable) {
            if ($server->get('ssl_port_default_server')) {
                if (!$this->validatePortDefaultServer($sslPort, $uuid, true)) {
                    throw new Exception('There is already another server set as the default on the selected SSL port.');
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
     * @param int    $port     The port to check.
     * @param string $uuid     ID of server to check.
     * @param string $hostType Indicate if the port is used in a port or
     *                         name-based setup.
     * @param bool   $ssl      Indicate wether the port is used as an SSL port.
     *
     * @return bool
     */
    private function validatePort($port, $uuid, $hostType, $ssl = false)
    {
        $webadminConfig = $this->database->get('conf.webadmin');

        $webadminPort = $webadminConfig->get('port');
        $webadminSslPort = $webadminConfig->get('sslport');
        $enableSsl = $webadminConfig->get('enablessl');

        // We don't need to check if force SSL only is enabled since the web
        // interface configuration still uses the http port to redirect to
        // https.
        //
        // If the host type is name-based we will still allow using the web
        // interface ports.
        if ($webadminPort === $port && ($hostType === self::HOST_TYPE_PORT || $ssl)) {
            return false;
        }

        if ($webadminSslPort === $port && $enableSsl && ($hostType === self::HOST_TYPE_PORT || !$ssl)) {
            return false;
        }

        // Get the server configurations matching the port to validage against.
        // When SSL is forced the non SSL port isn't used, so there's no need to
        // validate those entries.
        $serversPredicate = sprintf('port=\'%s\' and ssl_force=\'0\'', $port);
        $servers = $this->database->getByPredicate($this->serverDataModelPath, $serversPredicate);

        $sslServersPredicate = sprintf('ssl_port=\'%s\' and ssl_enable=\'1\'', $port);
        $sslServers = $this->database->getByPredicate($this->serverDataModelPath, $sslServersPredicate);

        if (!empty($servers)) {
            // SSL and non SSL port mismatch.
            if ($ssl) {
                return false;
            }

            $server = $servers[0];

            // Don't validate the server against itself.
            if ($server->get('uuid') !== $uuid) {
                // Check if the server is port-based.
                if ($hostType === self::HOST_TYPE_PORT || $server->get('host_type') === self::HOST_TYPE_PORT) {
                    return false;
                }
            }
        }

        if (!empty($sslServers)) {
            // Non SSL and SSL port mismatch.
            if (!$ssl) {
                return false;
            }

            $sslServer = $sslServers[0];

            if ($sslServer->get('uuid') !== $uuid) {
                if ($hostType === self::HOST_TYPE_PORT || $sslServer->get('host_type') === self::HOST_TYPE_PORT) {
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
     * @param int    $port The port number that should be checked.
     * @param string $uuid ID of the server configuration to be checked.
     * @param bool   $ssl  Indicate wether to validate against an SSL enabled
     *                     server or not. Defaults to false.
     *
     * @return bool
     */
    private function validatePortDefaultServer($port, $uuid, $ssl = false)
    {
        $webadminConfig = $this->database->get('conf.webadmin');

        if ($webadminConfig->get('port') === $port) {
            return false;
        }

        if ($webadminConfig->get('sslport') === $port) {
            return false;
        }

        $predicateFormat = 'port=\'%s\' and port_default_server=\'1\' and uuid!=\'%s\'';

        if ($ssl) {
            $predicate = 'ssl_port=\'%s\' and ssl_port_default_server=\'1\' and uuid!=\'%s\'';
        }

        $predicate = sprintf($predicateFormat, $port, $uuid);

        return !$this->database->existsByPredicate($this->serverDataModelPath, $predicate);
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
        $path = Rpc::call('ShareMgmt', 'getPath', [
            'uuid' => $uuid,
        ], $this->getAdminContext());

        return $path;
    }
}
