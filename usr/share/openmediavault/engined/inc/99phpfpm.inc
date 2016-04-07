<?php

/**
 * Copyright (C) 2015 OpenMediaVault Plugin Developers.
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

use OMV\System\LogFileSpec;

// Register new logfile type.
// Format:
// [01-Jan-2000 23:59:59] <message>
LogFileSpec::registerSpecification(
    'php-fpm',
    [
        'filename' => 'php-fpm',
        'filepath' => '/var/log/php5-fpm.log',
        'regex' => '/^\[(\d+-\w+-\d+\s+\d+:\d+:\d+)\]\s+(.*)$/',
        'columns' => [
            'ts' => [
                'index' => 1,
                'func' => function ($date) {
                    return strpdate($date, 'd-M-Y H:i:s');
                },
            ],
            'date' => [
                'index' => 1,
                'func' => function ($date) {
                    return strftime('%c', strpdate($date, 'd-M-Y H:i:s'));
                },
            ],
            'event' => 2,
        ],
    ]
);
