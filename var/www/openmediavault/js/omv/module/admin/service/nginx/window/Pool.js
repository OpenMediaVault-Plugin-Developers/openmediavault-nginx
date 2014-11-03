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

// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")
// require("js/omv/form/plugin/LinkedFields.js")
// require("js/omv/form/field/UserComboBox.js")

Ext.define("OMV.module.admin.service.nginx.window.Pool", {
    extend   : "OMV.workspace.window.Form",
    requires : [
        "OMV.workspace.window.plugin.ConfigObject",
        "OMV.form.plugin.LinkedFields",
        "OMV.form.field.UserComboBox"
    ],

    plugins : [{
        ptype : "configobject"
    }],

    rpcService   : "PhpFpm",
    rpcSetMethod : "set",

    height          : 600,
    hideResetButton : true,

    getFormItems : function() {
        var me = this;

        return [{
            xtype : "fieldset",
            title : _("General"),
            items : [{
                xtype      : "textfield",
                name       : "name",
                fieldLabel : _("Name"),
                allowBlank : false
            },{
                xtype      : "textfield",
                name       : "description",
                fieldLabel : _("Description"),
                allowBlank : true
            },{
                xtype      : "usercombo",
                name       : "user",
                fieldLabel : _("User"),
                userType   : "normal",
                editable   : false,
                onLoad     : Ext.Function.interceptBefore(OMV.form.field.UserComboBox.prototype, "onLoad", function(store, records, success) {
                    if (success) {
                        store.add({
                            name : "openmediavault"
                        },{
                            name : "www-data"
                        });
                    }
                }),
                plugins : [{
                    ptype : "fieldinfo",
                    text  : _("Set the user under which PHP scripts should be executed as.")
                }]
            },{
                xtype      : "groupcombo",
                name       : "group",
                fieldLabel : _("Group"),
                groupType  : "normal",
                editable   : false,
                onLoad     : Ext.Function.interceptBefore(OMV.form.field.GroupComboBox.prototype, "onLoad", function(store, records, success) {
                    if (success) {
                        store.add(
                            { name : "openmediavault" },
                            { name : "www-data" },
                            { name : "users" }
                        );
                    }
                }),
                plugins : [{
                    ptype : "fieldinfo",
                    text  : _("Set the group under which PHP scripts should be executed as.")
                }]
            },{
                xtype      : "checkbox",
                name       : "php_display_errors",
                fieldLabel : _("Display errors"),
                checked    : false
            },{
                xtype      : "checkbox",
                name       : "php_html_errors",
                fieldLabel : _("HTML errors"),
                checked    : true,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Use HTML tags in error messages."),
                }]
            },{
                xtype      : "numberfield",
                name       : "php_max_execution_time",
                fieldLabel : _("Max execution time (s)"),
                minValue   : 0,
                value      : 30,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Set the max execution time of a script. Using the value 0 means no limit."),
                }]
            },{
                xtype      : "numberfield",
                name       : "php_memory_limit",
                fieldLabel : _("Memory limit (MB)"),
                minValue   : -1,
                value      : 128,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("This setting should be higher than the post max size. Setting a value of -1 makes it unlimited."),
                }]
            },{
                xtype      : "numberfield",
                name       : "php_post_max_size",
                fieldLabel : _("Max POST size (MB)"),
                minValue   : 1,
                value      : 8,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("This setting affects the max upload filesize and should preferably be higher."),
                }],
                validator : function(value) {
                    var otherField = me.findField("php_upload_max_filesize");

                    if (value < otherField.getValue())
                        return "Value should be higher than max upload filesize";

                    return true;
                }
            },{
                xtype      : "numberfield",
                name       : "php_upload_max_filesize",
                fieldLabel : _("Max upload filesize (MB)"),
                minValue   : 1,
                value      : 2
            }]
        },{
            xtype : "fieldset",
            title : _("Process manager"),
            items : [{
                xtype      : "numberfield",
                name       : "pm_max_children",
                fieldLabel : _("Max children"),
                minValue   : 1,
                value      : 5
            },{
                xtype      : "numberfield",
                name       : "pm_start_servers",
                fieldLabel : _("Start servers"),
                minValue   : 1,
                value      : 2
            },{
                xtype      : "numberfield",
                name       : "pm_min_spare_servers",
                fieldLabel : _("Min spare servers"),
                minValue   : 1,
                value      : 1
            },{
                xtype      : "numberfield",
                name       : "pm_max_spare_servers",
                fieldLabel : _("Max spare servers"),
                minValue   : 1,
                value      : 3
            },{
                xtype      : "numberfield",
                name       : "pm_max_requests",
                fieldLabel : _("Max requests"),
                minValue   : 0,
                value      : 0
            }]
        },{
            xtype : "fieldset",
            title : _("Extra options"),
            items : [{
                xtype      : "textarea",
                name       : "extra_options",
                allowBlank : true
            }]
        }];
    }
});
