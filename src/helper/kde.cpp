#define __SP_KDE_CPP__

/*
 * KDE utilities for Sodipodi
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2003 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>

#include <qtimer.h>
#include <kapp.h>
#include <kfiledialog.h>
#include <gtk/gtkmain.h>

#include <helper/sp-intl.h>

#include "kde.h"
#include "kde-private.h"

#define SP_FOREIGN_FREQ 32
#define SP_FOREIGN_MAX_ITER 4

void
SPKDEBridge::EventHook (void) {
	int cdown = 0;
	while ((cdown++ < SP_FOREIGN_MAX_ITER) && gdk_events_pending ()) {
		gtk_main_iteration_do (FALSE);
	}
	gtk_main_iteration_do (FALSE);
}

void
SPKDEBridge::TimerHook (void) {
	int cdown = 10;
	while ((cdown++ < SP_FOREIGN_MAX_ITER) && gdk_events_pending ()) {
		gtk_main_iteration_do (FALSE);
	}
	gtk_main_iteration_do (FALSE);
}

static KApplication *KDESodipodi = NULL;
static SPKDEBridge *Bridge = NULL;
static bool SPKDEModal = FALSE;

static void
sp_kde_gdk_event_handler (GdkEvent *event)
{
	if (SPKDEModal) {
		// KDE widget is modal, filter events
		switch (event->type) {
		case GDK_NOTHING:
		case GDK_DELETE:
		case GDK_SCROLL:
		case GDK_BUTTON_PRESS:
		case GDK_2BUTTON_PRESS:
		case GDK_3BUTTON_PRESS:
		case GDK_BUTTON_RELEASE:
		case GDK_KEY_PRESS:
		case GDK_KEY_RELEASE:
		case GDK_DRAG_STATUS:
		case GDK_DRAG_ENTER:
		case GDK_DRAG_LEAVE:
		case GDK_DRAG_MOTION:
		case GDK_DROP_START:
		case GDK_DROP_FINISHED:
			return;
			break;
		default:
			break;
		}
	}
	gtk_main_do_event (event);
}

void
sp_kde_init (int argc, char **argv, const char *name)
{
	KDESodipodi = new KApplication (argc, argv, name);
	Bridge = new SPKDEBridge ("KDE Bridge");

	QObject::connect (KDESodipodi, SIGNAL (guiThreadAwake ()), Bridge, SLOT (EventHook ()));

	gdk_event_handler_set ((GdkEventFunc) sp_kde_gdk_event_handler, NULL, NULL);
}

void
sp_kde_finish (void)
{
	delete Bridge;
	delete KDESodipodi;
}

char *
sp_kde_get_open_filename (void)
{
	QString fileName;

	QTimer timer;
	QObject::connect (&timer, SIGNAL (timeout ()), Bridge, SLOT (TimerHook ()));
	timer.changeInterval (1000 / SP_FOREIGN_FREQ);
	SPKDEModal = TRUE;

	fileName = KFileDialog::getOpenFileName (QDir::currentDirPath (), _("*.svg *.svgz|SVG files\n*|All files"), NULL, _("Open SVG file"));

	SPKDEModal = FALSE;

        return g_strdup (fileName);
}

