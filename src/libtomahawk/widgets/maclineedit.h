/* This file is part of Clementine.
   Copyright 2010, David Sansome <me@davidsansome.com>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MACLINEEDIT_H
#define MACLINEEDIT_H

#include <QMacCocoaViewContainer>

#include "dllmacro.h"

class SearchTargetWrapper;

class DLLEXPORT LineEditInterface {
public:
  LineEditInterface(QWidget* widget) : widget_(widget) {}

  QWidget* widget() const { return widget_; }

  virtual ~LineEditInterface() {}

  virtual void clear() { set_text(QString()); }
  virtual void set_focus() = 0;
  virtual QString text() const = 0;
  virtual void set_text(const QString& text) = 0;

  virtual QString hint() const = 0;
  virtual void set_hint(const QString& hint) = 0;
  virtual void clear_hint() = 0;

  virtual void set_enabled(bool enabled) = 0;

protected:
  QWidget* widget_;
};

class DLLEXPORT MacLineEdit : public QMacCocoaViewContainer, public LineEditInterface {
  Q_OBJECT
  Q_PROPERTY(QString hint READ hint WRITE set_hint);

 public:
  MacLineEdit(QWidget* parent = 0);
  ~MacLineEdit();

  QString hint() const { return hint_; }
  void set_hint(const QString& hint);
  void clear_hint() { set_hint(QString()); }

  void paintEvent(QPaintEvent* e);

  void set_text(const QString&);
  QString text() const;
  void set_focus() {}

  void set_enabled(bool enabled);

 signals:
  void textChanged(const QString& text);
  void textEdited(const QString& text);

 private:
  // Called by NSSearchFieldCell when the text changes.
  void TextChanged(const QString& text);

  QString hint_;

  friend class SearchTargetWrapper;
  SearchTargetWrapper* wrapper_;
};

#endif // MACLINEEDIT_H
