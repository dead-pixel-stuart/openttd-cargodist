/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file news_widget.h Types related to the news widgets. */

#ifndef WIDGETS_NEWS_WIDGET_H
#define WIDGETS_NEWS_WIDGET_H

#include "../news_type.h"

/** Widgets of the #NewsWindow class. */
enum NewsWidgets {
	WID_N_PANEL,       ///< Panel of the window.
	WID_N_TITLE,       ///< Title of the company news.
	WID_N_HEADLINE,    ///< The news headline.
	WID_N_CLOSEBOX,    ///< Close the window.
	WID_N_DATE,        ///< Date of the news item.
	WID_N_CAPTION,     ///< Title bar of the window. Only used in small news items.
	WID_N_INSET,       ///< Inset around the viewport in the window. Only used in small news items.
	WID_N_VIEWPORT,    ///< Viewport in the window.
	WID_N_COMPANY_MSG, ///< Message in company news items.
	WID_N_MESSAGE,     ///< Space for displaying the message. Only used in small news items.
	WID_N_MGR_FACE,    ///< Face of the manager.
	WID_N_MGR_NAME,    ///< Name of the manager.
	WID_N_VEH_TITLE,   ///< Vehicle new title.
	WID_N_VEH_BKGND,   ///< Dark background of new vehicle news.
	WID_N_VEH_NAME,    ///< Name of the new vehicle.
	WID_N_VEH_SPR,     ///< Graphical display of the new vehicle.
	WID_N_VEH_INFO,    ///< Some technical data of the new vehicle.
};

/** Widgets of the #MessageHistoryWindow class. */
enum MessageHistoryWidgets {
	WID_MH_STICKYBOX,  ///< Stickybox.
	WID_MH_BACKGROUND, ///< Background of the window.
	WID_MH_SCROLLBAR,  ///< Scrollbar for the list.
};

/** Widgets of the #MessageOptionsWindow class. */
enum MessageOptionWidgets {
	WID_MO_BACKGROUND,        ///< Background of the window.
	WID_MO_LABEL,             ///< Top label.
	WID_MO_DROP_SUMMARY,      ///< Dropdown that adjusts at once the level for all settings.
	WID_MO_LABEL_SUMMARY,     ///< Label of the summary drop down.
	WID_MO_SOUNDTICKER,       ///< Button for (de)activating sound on events.
	WID_MO_SOUNDTICKER_LABEL, ///< Label of the soundticker button.

	WID_MO_START_OPTION,      ///< First widget that is part of a group [<][label][>] [description].
	WID_MO_END_OPTION = WID_MO_START_OPTION + NT_END * MOS_WIDG_PER_SETTING, ///< First widget after the groups.
};

#endif /* WIDGETS_NEWS_WIDGET_H */
