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

namespace OMV\Engined\Module;

use OMV\Engine\Module\Manager as ModuleManager;
use OMV\Engine\Module\ServiceAbstract;
use OMV\Engine\Notify\Dispatcher;
use OMV\Engine\Notify\IListener;

class NginxServer extends ServiceAbstract implements IListener
{
    /** @var string */
    private $eventMessagePath = 'org.openmediavault.conf.service.nginx';

    /** @var string */
    private $serverEventMessagePath = 'org.openmediavault.conf.service.nginx.server';

    /** @var string */
    private $sharedFolderEventMessagePath = 'org.openmediavault.conf.system.sharedfolder';

    /**
     * Get the module name.
     *
     * @return string
     */
    public function getName()
    {
        return 'nginx-server';
    }

    /**
     * Bind listeners.
     *
     * @param Dispatcher $dispatcher The dispatcher object.
     *
     * @return void
     */
    public function bindListeners(Dispatcher $dispatcher)
    {
        $moduleManager = ModuleManager::getInstance();

        $dispatcher->addListener(
            OMV_NOTIFY_CREATE | OMV_NOTIFY_MODIFY | OMV_NOTIFY_DELETE,
            $this->eventMessagePath,
            [$moduleManager->getModule('webserver'), 'setDirty']
        );

        $dispatcher->addListener(
            OMV_NOTIFY_CREATE | OMV_NOTIFY_MODIFY | OMV_NOTIFY_DELETE,
            $this->serverEventMessagePath,
            [$moduleManager->getModule('webserver'), 'setDirty']
        );

        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            $this->sharedFolderEventMessagePath,
            [$moduleManager->getModule('webserver'), 'setDirty']
        );
    }
}
