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

// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")
// require("js/omv/form/plugin/LinkedFields.js")
// require("js/omv/form/field/SharedFolderComboBox.js")
// require("js/omv/form/field/UserComboBox.js")

Ext.define('OMV.module.admin.service.nginx.window.Server', {
    extend: 'OMV.workspace.window.Form',
    requires: [
        'OMV.workspace.window.plugin.ConfigObject',
        'OMV.form.plugin.LinkedFields',
        'OMV.form.field.SharedFolderComboBox',
        'OMV.form.field.UserComboBox'
    ],

    plugins: [{
        ptype: 'configobject'
    }, {
        ptype: 'linkedfields',
        correlations: [{
            name: [
                'use_root',
            ],
            conditions: [{
                name: 'sharedfolderref',
                op: 'z'
            }],
            properties: [
                'readOnly'
            ]
        }, {
            name: [
                'use_public_directory'
            ],
            conditions: [{
                name: 'use_root',
                value: true
            }, {
                name: 'sharedfolderref',
                op: 'n'
            }],
            properties: [
                '!readOnly'
            ]
        }, {
            name: [
                'public_directory'
            ],
            conditions: [{
                name: 'use_public_directory',
                value: true
            }],
            properties: [
                'show',
                '!allowBlank',
                '!readOnly'
            ]
        }, {
            name: [
                'server_name'
            ],
            conditions: [{
                name: 'host_type',
                value: 'name'
            }],
            properties: [
                'show',
                '!allowBlank',
                '!readOnly'
            ]
        }, {
            name: [
                'port',
                'port_default_server',
            ],
            conditions: [{
                name: 'ssl_force',
                value: true
            }],
            properties: [
                'allowBlank',
                'readOnly',
            ]
        }, {
            name: [
                'sslcertificateref',
                'ssl_port',
                'ssl_port_default_server',
                'ssl_force'
            ],
            conditions: [{
                name: 'ssl_enable',
                value: true
            }],
            properties: [
                'show',
                '!allowBlank',
                '!allowNone',
                '!readOnly'
            ]
        }, {
            name: [
                'php_pool_ref',
                'php_use_default_config',
            ],
            conditions: [{
                name: 'php_enable',
                value: true
            }],
            properties: [
                '!allowBlank',
                '!allowNone',
                '!readOnly',
                'show'
            ]
        }, {
            name: [
                'use_index_html',
                'use_index_php',
            ],
            conditions: [{
                name: 'use_index',
                value: true
            }],
            properties: [
                '!readOnly'
            ]
        }]
    }],

    rpcService: 'Nginx',
    rpcSetMethod: 'set',

    height: 600,
    hideResetButton: true,

    getFormItems: function() {
        return [{
            xtype: 'fieldset',
            title: _('General'),
            items: [{
                xtype: 'checkbox',
                name: 'enable',
                fieldLabel: _('Enable'),
                checked: true
            }, {
                xtype: 'sharedfoldercombo',
                name: 'sharedfolderref',
                fieldLabel: _('Directory'),
                allowBlank: true,
                allowNone: true,
                value: '',
                plugins: [{
                    ptype: 'fieldinfo',
                    text: _('The location needs to have at least read permissions for the user/group www-data.') + ' ' +
                        _('This option will populate the $root_path variable for usage in extra options or as document root.')
                }]
            }, {
                xtype: 'checkbox',
                name: 'use_root',
                fieldLabel: _('Use root'),
                checked: true,
                plugins: [{
                    ptype: 'fieldinfo',
                    text: _('Use "Directory" as document root.')
                }]
            }, {
                xtype: 'checkbox',
                name: 'use_public_directory',
                fieldLabel: _('Use public directory'),
                checked: false,
                readOnly: true,
                plugins: [{
                    ptype: 'fieldinfo',
                    text: _('Use if you serve your public files in a subfolder like public_html or public. This folder will be the document root. It also avoids the need to create an additional shared folder in such cases.')
                }]
            }, {
                xtype: 'combo',
                name: 'public_directory',
                fieldLabel: _('Public directory'),
                queryMode: 'local',
                store: Ext.create('Ext.data.ArrayStore', {
                    fields: [
                        'value'
                    ],
                    data: [
                        ['public_html'],
                        ['public'],
                    ]
                }),
                displayField: 'value',
                valueField: 'value',
                allowBlank: false,
                editable: true,
                triggerAction: 'all',
                value: 'public_html'
            }, {
                xtype: 'combo',
                name: 'host_type',
                fieldLabel: _('Host type'),
                queryMode: 'local',
                store: Ext.create('Ext.data.ArrayStore', {
                    fields: [
                        'value',
                        'text'
                    ],
                    data: [
                        ['port', _('Port')],
                        ['name', _('Name-based')]
                    ]
                }),
                displayField: 'text',
                valueField: 'value',
                allowBlank: false,
                editable: false,
                triggerAction: 'all',
                value: 'port'
            }, {
                xtype: 'textfield',
                name: 'server_name',
                fieldLabel: _('Server name'),
                allowBlank: true,
                readOnly: true,
                hidden: true
            }, {
                xtype: 'numberfield',
                name: 'port',
                fieldLabel: _('Port'),
                vtype: 'port',
                minValue: 0,
                maxValue: 65535,
                allowDecimals: false,
                allowNegative: false,
                value: 80
            }, {
                xtype: 'checkbox',
                name: 'port_default_server',
                fieldLabel: _('Default server'),
                checked: false,
                plugins: [{
                    ptype: 'fieldinfo',
                    text: _('Set the server as the default one to be served if no server name matches on the selected port.')
                }]
            }]
        }, {
            xtype: 'fieldset',
            title: 'SSL',
            items: [{
                xtype: 'checkbox',
                name: 'ssl_enable',
                fieldLabel: _('Enable SSL'),
                checked: false,
                listeners: {
                    change: function(field, newValue) {
                        var sslForceField = this.findField('ssl_force');
                        sslForceField.setValue(false);
                    },
                    scope: this
                }
            }, {
                xtype: 'numberfield',
                name: 'ssl_port',
                fieldLabel: _('Port'),
                vtype: 'port',
                minValue: 0,
                maxValue: 65535,
                allowDecimals: false,
                allowNegative: false,
                value: 443
            }, {
                xtype: 'checkbox',
                name: 'ssl_port_default_server',
                fieldLabel: _('Default server'),
                checked: false,
                plugins: [{
                    ptype: 'fieldinfo',
                    text: _('Set the server as the default one to be served if no server name matches on the selected port.')
                }]
            }, {
                xtype: 'sslcertificatecombo',
                name: 'sslcertificateref',
                fieldLabel: _('Certificate'),
                allowNone: true,
                allowBlank: true,
                readOnly: true,
                hidden: true,
                value: '',
                plugins: [{
                    ptype: 'fieldinfo',
                    text: _('The SSL certificate.')
                }]
            }, {
                xtype: 'checkbox',
                name: 'ssl_force',
                fieldLabel: _('Only use SSL'),
                checked: false
            }]
        }, {
            xtype: 'fieldset',
            title: 'PHP',
            items: [{
                xtype: 'checkbox',
                name: 'php_enable',
                fieldLabel: _('Enable PHP'),
                checked: false,
                plugins: [{
                    ptype: 'fieldinfo',
                    text: _('This enables PHP and makes the $socket variable (for the chosen PHP-FPM pool) available in extra options.')
                }]
            }, {
                xtype: 'combo',
                name: 'php_pool_ref',
                fieldLabel: _('PHP-FPM pool'),
                emptyText: _('Select a PHP-FPM pool ...'),
                allowBlank: false,
                allowNone: false,
                editable: false,
                triggerAction: 'all',
                displayField: 'name',
                valueField: 'uuid',
                store: Ext.create('OMV.data.Store', {
                    autoLoad: true,
                    model: OMV.data.Model.createImplicit({
                        idProperty: 'uuid',
                        fields: [{
                            name: 'uuid',
                            type: 'string'
                        }, {
                            name: 'name',
                            type: 'string'
                        }]
                    }),
                    proxy: {
                        type: 'rpc',
                        rpcData: {
                            service: 'PhpFpm',
                            method: 'getList'
                        },
                    },
                    sorters: [{
                        direction: 'ASC',
                        property: 'name'
                    }]
                }),
                value: ''
            }, {
                xtype: 'checkbox',
                name: 'php_use_default_config',
                fieldLabel: _('Default config'),
                checked: true,
                plugins: [{
                    ptype: 'fieldinfo',
                    text: _('Use the default Nginx config for PHP.') + ' ' +
                        _('Uncheck this if you want to use your own configuration to connect to the PHP socket.')
                }]
            }]
        }, {
            xtype: 'fieldset',
            title: _('Options'),
            items: [{
                xtype: 'fieldset',
                title: _('Index'),
                items: [{
                    xtype: 'checkbox',
                    name: 'use_index',
                    boxLabel: _('Use index'),
                    checked: true,
                }, {
                    border: false,
                    layout: 'column',
                    defaults: {
                        border: false,
                        columnWidth: 0.5,
                        hideLabel: true,
                        labelSeparator: '',
                        layout: 'form'
                    },
                    items: [{
                        items: [{
                            xtype: 'checkbox',
                            name: 'use_index_html',
                            boxLabel: 'index.html',
                            checked: true
                        }]
                    }, {
                        items: [{
                            xtype: 'checkbox',
                            name: 'use_index_php',
                            boxLabel: 'index.php',
                            checked: false
                        }]
                    }]
                }]
            }, {
                border: false,
                layout: 'column',
                defaults: {
                    border: false,
                    columnWidth: 0.5,
                    hideLabel: true,
                    labelSeparator: '',
                    layout: 'form'
                },
                items: [{
                    items: [{
                        xtype: 'checkbox',
                        name: 'autoindex',
                        boxLabel: 'Autoindex',
                        checked: false
                    }, {
                        xtype: 'checkbox',
                        name: 'log_enable',
                        fieldLabel: _('Enable log'),
                        checked: true
                    }]
                }, {
                    items: [{
                        xtype: 'checkbox',
                        name: 'deny_htaccess',
                        boxLabel: _('Don\'t serve .htaccess'),
                        checked: false
                    }]
                }]
            }, {
                xtype: 'combo',
                name: 'large_client_header_buffers',
                fieldLabel: _('Header buffer size'),
                queryMode: 'local',
                store: Ext.create('Ext.data.ArrayStore', {
                    fields: [
                        'value',
                        'text'
                    ],
                    data: [
                        [8, '8k'],
                        [16, '16k'],
                        [32, '32k'],
                    ]
                }),
                displayField: 'text',
                valueField: 'value',
                allowBlank: false,
                editable: false,
                triggerAction: 'all',
                value: 8
            }]
        }, {
            xtype: 'fieldset',
            title: _('Extra options'),
            items: [{
                xtype: 'textarea',
                name: 'extra_options',
                minHeight: 150,
                allowBlank: true
            }]
        }];
    }
});
