# vim:set noet ts=4:
#
# ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
#
# Copyright (c) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
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

WORDLIST = wordlist
ENGLISH_AWK = english.awk
ENGLISH_DB = english.db

STROKES = strokes
STROKES_AWK = strokes.awk
STROKES_DB = strokes.db

SUBDIRS = \
	icons \
	$(NULL)

auxiliary_db_DATA = \
        $(ENGLISH_DB) \
        $(STROKES_DB) \
        $(NULL)
auxiliary_dbdir = $(pkgdatadir)/db

$(ENGLISH_DB): $(WORDLIST) $(ENGLISH_AWK)
	$(AM_V_GEN) \
	$(RM) $@; \
	$(AWK) -f $(srcdir)/$(ENGLISH_AWK) $(srcdir)/$(WORDLIST) | @SQLITE3@ $@ || \
		( $(RM) $@ ; exit 1 )

$(STROKES_DB): $(STROKES) $(STROKES_AWK)
	$(AM_V_GEN) \
	$(RM) $@; \
	$(AWK) -f $(srcdir)/$(STROKES_AWK) $(srcdir)/$(STROKES) | @SQLITE3@ $@ || \
		( $(RM) $@ ; exit 1 )

desktop_in_in_files = \
	ibus-setup-libbopomofo.desktop.in.in \
	ibus-setup-libpinyin.desktop.in.in \
	$(NULL)

desktop_in_files = $(desktop_in_in_files:.in.in=.in)

$(desktop_in_files): %.desktop.in: %.desktop.in.in Makefile
	$(AM_V_GEN) sed \
		-e "s|\@libexecdir\@|$(libexecdir)|" \
		-e "s|\@pkgdatadir\@|$(pkgdatadir)|" \
		$< > $@.tmp && mv $@.tmp $@

desktopdir=$(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(desktop_in_in_files) \
	$(WORDLIST) \
	$(ENGLISH_AWK) \
	$(STROKES) \
	$(STROKES_AWK) \
	$(NULL)

CLEANFILES = \
	$(ENGLISH_DB) \
	$(STROKES_DB) \
	$(desktop_in_files) \
	$(desktop_DATA) \
	$(NULL)
