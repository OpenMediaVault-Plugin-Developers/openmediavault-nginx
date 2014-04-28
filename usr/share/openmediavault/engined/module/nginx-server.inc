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

require_once "openmediavault/module.inc";
require_once "openmediavault/config.inc";
require_once "openmediavault/error.inc";
require_once "openmediavault/util.inc";
require_once "openmediavault/initscript.inc";

class OMVModuleNginxServer extends OMVModuleServiceAbstract
    implements OMVINotifyListener
{
    private function getEventMessagePath()
    {
        return "org.openmediavault.services.nginx";
    }

    /**
     * Get the module name.
     */
    public function getName()
    {
        return "nginx-server";
    }

    /**
     * Bind listeners.
     */
    public function bindListeners(OMVNotifyDispatcher $dispatcher) {
        $moduleMgr = &OMVModuleMgr::getInstance();

        $dispatcher->addListener(
            OMV_NOTIFY_CREATE | OMV_NOTIFY_MODIFY | OMV_NOTIFY_DELETE,
            $this->getEventMessagePath(),
            array($moduleMgr->getModule("webserver"), "setDirty"));

        $dispatcher->addListener(
            OMV_NOTIFY_CREATE | OMV_NOTIFY_MODIFY | OMV_NOTIFY_DELETE,
            $this->getEventMessagePath(),
            array($moduleMgr->getModule("php5fpm"), "setDirty"));
    }
}

// Register module
$moduleMgr = &OMVModuleMgr::getInstance();
$moduleMgr->registerModule(new OMVModuleNginxServer());

// Register new logfile types.
OMVLogFile::registerType("php-fpm",
    array(
        "filename" => "php-fpm",
        "filepath" => "/var/log/php5-fpm.log",
        "regex"    => "/^\[(\d+-\w+-\d+\s+\d+:\d+:\d+)\]\s+(.*)$/",
        "columns"  => array(
            "ts" => array(
                "index" => 1,
                "func"  => create_function("\$v",
                    "return strpdate(\$v, \"d-M-Y H:i:s\");")
            ),
            "date" => array(
                "index" => 1,
                "func"  => create_function('$v',
                    "return strftime(\"%c\", strpdate(\$v,\"d-M-Y H:i:s\"));")
            ),
            "event" => 2
        )
    )
);

