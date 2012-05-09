/*======================================================== 
**University of Illinois/NCSA 
**Open Source License 
**
**Copyright (C) 2007-2008,The Board of Trustees of the University of 
**Illinois. All rights reserved. 
**
**Developed by: 
**
**    Research Group of Professor Sam King in the Department of Computer 
**    Science The University of Illinois at Urbana-Champaign 
**    http://www.cs.uiuc.edu/homes/kingst/Research.html 
**
**Permission is hereby granted, free of charge, to any person obtaining a 
**copy of this software and associated documentation files (the 
**¡°Software¡±), to deal with the Software without restriction, including 
**without limitation the rights to use, copy, modify, merge, publish, 
**distribute, sublicense, and/or sell copies of the Software, and to 
**permit persons to whom the Software is furnished to do so, subject to 
**the following conditions: 
**
*** Redistributions of source code must retain the above copyright notice, 
**this list of conditions and the following disclaimers. 
*** Redistributions in binary form must reproduce the above copyright 
**notice, this list of conditions and the following disclaimers in the 
**documentation and/or other materials provided with the distribution. 
*** Neither the names of <Name of Development Group, Name of Institution>, 
**nor the names of its contributors may be used to endorse or promote 
**products derived from this Software without specific prior written 
**permission. 
**
**THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
**EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
**MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
**IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
**ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
**TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
**SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE. 
**========================================================== 
*/
/*
 * Copyright 2007-2008 Benjamin C. Meyer <ben@meyerhome.net>
 * Copyright 2008 Matvey Kozhev <sikon@ubuntu.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "aboutdialog.h"

#include <qdialogbuttonbox.h>
#include <qfile.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtextedit.h>
#include <qtextstream.h>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(tr("About") + QLatin1String(" ") + qApp->applicationName());
    logo->setPixmap(qApp->windowIcon().pixmap(128, 128));
    name->setText(qApp->applicationName());
    version->setText(QApplication::applicationVersion());
    connect(authorsButton, SIGNAL(clicked()),
            this, SLOT(authorsButtonClicked()));
    connect(licenseButton, SIGNAL(clicked()),
            this, SLOT(licenseButtonClicked()));
}

void AboutDialog::displayFile(const QString &fileName, const QString &title)
{
    QDialog *dialog = new QDialog(this);
    QLayout *layout = new QVBoxLayout(dialog);
    QTextEdit *textEdit = new QTextEdit(dialog);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, dialog);

    textEdit->setStyleSheet(QLatin1String("font-family: monospace"));

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QString text = QTextStream(&file).readAll();
        textEdit->setPlainText(text);
    }

    textEdit->setReadOnly(true);
    connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(close()));
    buttonBox->setCenterButtons(true);
    layout->addWidget(textEdit);
    layout->addWidget(buttonBox);
    layout->setMargin(6);

    dialog->setLayout(layout);
    dialog->setWindowTitle(title);
    dialog->setWindowFlags(Qt::Sheet);
    dialog->resize(600, 350);
    dialog->exec();
}

void AboutDialog::authorsButtonClicked()
{
    displayFile(QLatin1String(":AUTHORS"), tr("Authors"));
}

void AboutDialog::licenseButtonClicked()
{
    displayFile(QLatin1String(":LICENSE.GPL2"), tr("License"));
}

