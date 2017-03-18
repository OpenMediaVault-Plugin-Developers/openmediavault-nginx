/**
 * Copyright (C) 2014-2017 OpenMediaVault Plugin Developers
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
// require("js/omv/workspace/grid/Panel.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/data/Store.js")
// require("js/omv/module/admin/service/nginx/window/Server.js")
// require("js/omv/module/admin/service/nginx/window/Log.js")

Ext.define('OMV.module.admin.service.nginx.Servers', {
    extend: 'OMV.workspace.grid.Panel',
    requires: [
        'OMV.data.Model',
        'OMV.data.proxy.Rpc',
        'OMV.data.Store',
        'OMV.module.admin.service.nginx.window.Log',
        'OMV.module.admin.service.nginx.window.Server'
    ],

    hidePagingToolbar: false,
    accessLogButtonText: _('Access log'),
    errorLogButtonText: _('Error log'),
    reloadOnActivate: true,

    columns: [{
        xtype: "textcolumn",
        header: _('UUID'),
        hidden: true,
        dataIndex: 'uuid'
    }, {
        xtype: 'booleaniconcolumn',
        header: _('Enabled'),
        sortable: true,
        dataIndex: 'enable',
        align: 'center',
        width: 80,
        resizable: false,
        trueIcon: 'switch_on.png',
        falseIcon: 'switch_off.png'
    }, {
        xtype: 'booleaniconcolumn',
        header: 'SSL',
        sortable: true,
        dataIndex: 'ssl_enable',
        align: 'center',
        width: 60,
        resizable: false,
        trueIcon: 'switch_on.png',
        falseIcon: 'switch_off.png'
    }, {
        xtype: 'booleaniconcolumn',
        header: 'PHP',
        sortable: true,
        dataIndex: 'php_enable',
        align: 'center',
        width: 60,
        resizable: false,
        trueIcon: 'switch_on.png',
        falseIcon: 'switch_off.png'
    }, {
        xtype: 'booleaniconcolumn',
        header: _('Default'),
        sortable: true,
        dataIndex: 'port_default_server',
        align: 'center',
        width: 60,
        resizable: false,
        trueIcon: 'switch_on.png',
        falseIcon: 'switch_off.png'
    }, {
        xtype: 'booleaniconcolumn',
        header: _('Default (SSL)'),
        sortable: true,
        dataIndex: 'ssl_port_default_server',
        align: 'center',
        width: 80,
        resizable: false,
        trueIcon: 'switch_on.png',
        falseIcon: 'switch_off.png'
    }, {
        xtype: "textcolumn",
        header: _('Root'),
        flex: 1,
        sortable: true,
        dataIndex: 'root_full_path'
    }, {
        xtype: "textcolumn",
        header: _('URL'),
        flex: 1,
        sortable: true,
        dataIndex: 'urls',
        renderer: function(urls) {
            var value = '';

            for (var i = 0; i < urls.length; i++)
                urls[i] = Ext.String.format('<a href="{0}" target="_blank">{0}</a>', urls[i]
                    .replace('!domain!', window.location.hostname));

            value = urls.join(', ');

            return value;
        }
    }],

    store: Ext.create('OMV.data.Store', {
        autoLoad: true,
        model: OMV.data.Model.createImplicit({
            idProperty: 'uuid',
            fields: [{
                name: 'uuid'
            }, {
                name: 'enable'
            }, {
                name: 'log_enable'
            }, {
                name: 'ssl_enable'
            }, {
                name: 'php_enable'
            }, {
                name: 'port_default_server'
            }, {
                name: 'ssl_port_default_server'
            }, {
                name: 'root_full_path'
            }, {
                name: 'urls'
            }]
        }),
        proxy: {
            type: 'rpc',
            rpcData: {
                'service': 'Nginx',
                'method': 'getList'
            }
        },
        remoteSort: true
    }),

    getTopToolbarItems: function() {
        var items = this.callParent(arguments);

        Ext.Array.push(items, [{
            xtype: 'tbseparator'
        }, {
            id: this.getId() + '-access-log',
            xtype: 'button',
            text: this.accessLogButtonText,
            handler: Ext.Function.bind(this.onViewLogButton, this, ['access']),
            scope: this,
            disabled: true,
            selectionConfig: {
                minSelections: 1,
                maxSelections: 1,
                enabledFn: Ext.Function.bind(this.logButtonEnabled, this)
            }
        }, {
            id: this.getId() + '-error-log',
            xtype: 'button',
            text: this.errorLogButtonText,
            handler: Ext.Function.bind(this.onViewLogButton, this, ['error']),
            scope: this,
            disabled: true,
            selectionConfig: {
                minSelections: 1,
                maxSelections: 1,
                enabledFn: Ext.Function.bind(this.logButtonEnabled, this)
            }
        }]);

        return items;
    },

    logButtonEnabled: function(button, records) {
        var record = records[0];

        return record.get('log_enable');
    },

    onAddButton: function() {
        Ext.create('OMV.module.admin.service.nginx.window.Server', {
            title: _('Add server'),
            uuid: OMV.UUID_UNDEFINED,
            listeners: {
                scope: this,
                submit: function() {
                    this.doReload();
                }
            }
        }).show();
    },

    onEditButton: function() {
        var record = this.getSelected();

        Ext.create('OMV.module.admin.service.nginx.window.Server', {
            rpcGetMethod: 'get',
            title: _('Edit server'),
            uuid: record.get('uuid'),
            listeners: {
                scope: this,
                submit: function() {
                    this.doReload();
                }
            }
        }).show();
    },

    doDeletion: function(record) {
        OMV.Rpc.request({
            scope: this,
            callback: this.onDeletion,
            rpcData: {
                service: 'Nginx',
                method: 'delete',
                params: {
                    uuid: record.get('uuid')
                }
            }
        });
    },

    onViewLogButton: function(logType) {
        var record = this.getSelected();

        Ext.create('OMV.module.admin.service.nginx.window.Log', {
            uuid: record.get('uuid'),
            logType: logType
        }).show();
    }
});

OMV.WorkspaceManager.registerPanel({
    id: 'servers',
    path: '/service/nginx',
    text: _('Servers'),
    position: 20,
    className: 'OMV.module.admin.service.nginx.Servers'
});
