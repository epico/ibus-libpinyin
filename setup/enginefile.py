# vim:set et ts=4 sts=4:
# -*- coding: utf-8 -*-
#
# ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
#
# Copyright (c) 2024 Peng Wu <alexepico@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

import os
import xml.dom.minidom
import codecs

from gi import require_version as gi_require_version
gi_require_version('GLib', '2.0')
gi_require_version('Gio', '2.0')

from gi.repository import GLib
from gi.repository import Gio

pkgdatadir = os.getenv("IBUS_PKGDATADIR") or "."

def save_layout():
    # assume the name and layout tag has the same order,
    # save both libpinyin and libbopomofo value here.
    system_config = os.path.join(pkgdatadir, 'default.xml')

    dom = xml.dom.minidom.parse(system_config)
    names = dom.getElementsByTagName('name')
    layouts = dom.getElementsByTagName('layout')
    assert len(names) == len(layouts)
    for i, name in enumerate(names):
        engine = name.childNodes[0].data
        config_namespace = "com.github.libpinyin.ibus-libpinyin." + engine
        config = Gio.Settings.new(config_namespace)
        var = config.get_value("keyboard-layout")
        assert 's' == var.get_type_string()
        layout = var.get_string()
        layouts[i].childNodes[0].data = layout

    user_config = os.path.join(GLib.get_user_config_dir(),
                               'ibus', 'libpinyin', 'engines.xml')
    dir = os.path.dirname(user_config)
    os.path.exists(dir) or os.makedirs(dir, 0o700)
    # io.open() causes TypeError for unicode.
    f = codecs.open(user_config, 'w', 'utf-8')
    dom.writexml(f, '', '', '', 'utf-8')
    f.close()
    os.chmod(user_config, 0o600)

def resync_engine_file():
    user_config = os.path.join(GLib.get_user_config_dir(),
                               'ibus', 'libpinyin', 'engines.xml')
    system_config = os.path.join(pkgdatadir, 'default.xml')
    if not os.path.exists(user_config):
        return
    if not os.path.exists(system_config):
        os.unlink(user_config)
        return

    # path.getmtime depends on the build time rather than install time.
    def __get_engine_file_version(engine_file):
        version_str = ''
        dom = xml.dom.minidom.parse(engine_file)
        # there are two versions in the engines.xml, only use the first one
        elements = dom.getElementsByTagName('version')
        nodes = []
        if len(elements) > 0:
            nodes = elements[0].childNodes
        if len(nodes) > 0:
            version_str = nodes[0].data
        if version_str != '':
            version_str = version_str.strip()
        return version_str

    user_config_version = __get_engine_file_version(user_config)
    system_config_version = __get_engine_file_version(system_config)
    if system_config_version != user_config_version:
        # generate the user config from gsettings
        save_layout()
