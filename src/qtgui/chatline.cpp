/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "chatline.h"

//!\brief Construct a ChatLine object from a message.
/**
 * \param m   The message to be layouted and rendered
 * \param net The network name
 * \param buf The buffer name
 */
ChatLine::ChatLine(Message m) {
  hght = 0;
  //networkName = m.buffer.network();
  //bufferName = m.buffer.buffer();
  msg = m;
  selectionMode = None;
  formatMsg(msg);
}

ChatLine::~ChatLine() {

}

void ChatLine::formatMsg(Message msg) {
  QString user = userFromMask(msg.sender());
  QString host = hostFromMask(msg.sender());
  QString nick = nickFromMask(msg.sender());
  QString text = Style::mircToInternal(msg.text());
  QString networkName = msg.buffer().network();
  QString bufferName = msg.buffer().buffer();

  QString c = tr("%DT[%1]").arg(msg.timeStamp().toLocalTime().toString("hh:mm:ss"));
  QString s, t;
  switch(msg.type()) {
    case Message::Plain:
      s = tr("%DS<%1>").arg(nick); t = tr("%D0%1").arg(text); break;
    case Message::Server:
      s = tr("%Ds*"); t = tr("%Ds%1").arg(text); break;
    case Message::Error:
      s = tr("%De*"); t = tr("%De%1").arg(text); break;
    case Message::Join:
      s = tr("%Dj-->"); t = tr("%Dj%DN%DU%1%DU%DN %DH(%2@%3)%DH has joined %DC%DU%4%DU%DC").arg(nick, user, host, bufferName); break;
    case Message::Part:
      s = tr("%Dp<--"); t = tr("%Dp%DN%DU%1%DU%DN %DH(%2@%3)%DH has left %DC%DU%4%DU%DC").arg(nick, user, host, bufferName);
      if(!text.isEmpty()) t = QString("%1 (%2)").arg(t).arg(text);
      break;
    case Message::Quit:
      s = tr("%Dq<--"); t = tr("%Dq%DN%DU%1%DU%DN %DH(%2@%3)%DH has quit").arg(nick, user, host);
      if(!text.isEmpty()) t = QString("%1 (%2)").arg(t).arg(text);
      break;
    case Message::Kick:
    { s = tr("%Dk<-*");
    QString victim = text.section(" ", 0, 0);
        //if(victim == ui.ownNick->currentText()) victim = tr("you");
    QString kickmsg = text.section(" ", 1);
    t = tr("%Dk%DN%DU%1%DU%DN has kicked %DN%DU%2%DU%DN from %DC%DU%3%DU%DC").arg(nick).arg(victim).arg(bufferName);
    if(!kickmsg.isEmpty()) t = QString("%1 (%2)").arg(t).arg(kickmsg);
    }
    break;
    case Message::Nick:
      s = tr("%Dr<->");
      if(nick == msg.text()) t = tr("%DrYou are now known as %DN%1%DN").arg(msg.text());
      else t = tr("%Dr%DN%1%DN is now known as %DN%DU%2%DU%DN").arg(nick, msg.text());
      break;
    case Message::Mode:
      s = tr("%Dm***");
      if(nick.isEmpty()) t = tr("%DmUser mode: %DM%1%DM").arg(msg.text());
      else t = tr("%DmMode %DM%1%DM by %DN%DU%2%DU%DN").arg(msg.text(), nick);
      break;
    case Message::Action:
      s = tr("%Da-*-");
      t = tr("%Da%DN%DU%1%DU%DN %2").arg(nick).arg(msg.text());
      break;
    default:
      s = tr("%De%1").arg(msg.sender());
      t = tr("%De[%1]").arg(msg.text());
  }
  QTextOption tsOption, senderOption, textOption;
  tsFormatted = Style::internalToFormatted(c);
  senderFormatted = Style::internalToFormatted(s);
  textFormatted = Style::internalToFormatted(t);
  precomputeLine();
}

QList<ChatLine::FormatRange> ChatLine::calcFormatRanges(const Style::FormattedString &fs, QTextLayout::FormatRange additional) {
  QList<FormatRange> ranges;
  QList<QTextLayout::FormatRange> formats = fs.formats;
  formats.append(additional);
  int cur = -1;
  FormatRange range, lastrange;
  for(int i = 0; i < fs.text.length(); i++) {
    QTextCharFormat format;
    foreach(QTextLayout::FormatRange f, formats) {
      if(i >= f.start && i < f.start + f.length) format.merge(f.format);
    }
    if(cur < 0) {
      range.start = 0; range.length = 1; range.format= format;
      cur = 0;
    } else {
      if(format == range.format) range.length++;
      else {
        QFontMetrics metrics(range.format.font());
        range.height = metrics.lineSpacing();
        ranges.append(range);
        range.start = i; range.length = 1; range.format = format;
        cur++;
      }
    }
  }
  if(cur >= 0) {
    QFontMetrics metrics(range.format.font());
    range.height = metrics.lineSpacing();
    ranges.append(range);
  }
  return ranges;
}

void ChatLine::setSelection(SelectionMode mode, int start, int end) {
  selectionMode = mode;
  //tsFormat.clear(); senderFormat.clear(); textFormat.clear();
  QPalette pal = QApplication::palette();
  QTextLayout::FormatRange tsSel, senderSel, textSel;
  switch (mode) {
    case None:
      tsFormat = calcFormatRanges(tsFormatted);
      senderFormat = calcFormatRanges(senderFormatted);
      textFormat = calcFormatRanges(textFormatted);
      break;
    case Partial:
      selectionStart = qMin(start, end); selectionEnd = qMax(start, end);
      textSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      textSel.format.setBackground(pal.brush(QPalette::Highlight));
      textSel.start = selectionStart;
      textSel.length = selectionEnd - selectionStart;
      //textFormat.append(textSel);
      textFormat = calcFormatRanges(textFormatted, textSel);
      foreach(FormatRange fr, textFormat);
      break;
    case Full:
      tsSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      tsSel.format.setBackground(pal.brush(QPalette::Highlight));
      tsSel.start = 0; tsSel.length = tsFormatted.text.length();
      tsFormat = calcFormatRanges(tsFormatted, tsSel);
      senderSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      senderSel.format.setBackground(pal.brush(QPalette::Highlight));
      senderSel.start = 0; senderSel.length = senderFormatted.text.length();
      senderFormat = calcFormatRanges(senderFormatted, senderSel);
      textSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      textSel.format.setBackground(pal.brush(QPalette::Highlight));
      textSel.start = 0; textSel.length = textFormatted.text.length();
      textFormat = calcFormatRanges(textFormatted, textSel);
      break;
  }
}

uint ChatLine::msgId() const {
  return msg.buffer().uid();
}

BufferId ChatLine::bufferId() const {
  return msg.buffer();
}

QDateTime ChatLine::timeStamp() const {
  return msg.timeStamp();
}

QString ChatLine::sender() const {
  return senderFormatted.text;
}

QString ChatLine::text() const {
  return textFormatted.text;
}

bool ChatLine::isUrl(int c) const {
  if(c < 0 || c >= charUrlIdx.count()) return false;;
  return charUrlIdx[c] >= 0;
}

QUrl ChatLine::getUrl(int c) const {
  if(c < 0 || c >= charUrlIdx.count()) return QUrl();
  int i = charUrlIdx[c];
  if(i >= 0) return textFormatted.urls[i].url;
  else return QUrl();
}

//!\brief Return the cursor position for the given coordinate pos.
/**
 * \param pos The position relative to the ChatLine
 * \return The cursor position, [or -3 for invalid,] or -2 for timestamp, or -1 for sender
 */
int ChatLine::posToCursor(QPointF pos) {
  if(pos.x() < tsWidth + (int)Style::sepTsSender()/2) return -2;
  qreal textStart = tsWidth + Style::sepTsSender() + senderWidth + Style::sepSenderText();
  if(pos.x() < textStart) return -1;
  int x = (int)(pos.x() - textStart);
  for(int l = lineLayouts.count() - 1; l >=0; l--) {
    LineLayout line = lineLayouts[l];
    if(pos.y() >= line.y) {
      int offset = charPos[line.start]; x += offset;
      for(int i = line.start + line.length - 1; i >= line.start; i--) {
        if((charPos[i] + charPos[i+1])/2 <= x) return i+1; // FIXME: Optimize this!
      }
      return line.start;
    }
  }
  return 0;
}

void ChatLine::precomputeLine() {
  tsFormat = calcFormatRanges(tsFormatted);
  senderFormat = calcFormatRanges(senderFormatted);
  textFormat = calcFormatRanges(textFormatted);

  minHeight = 0;
  foreach(FormatRange fr, tsFormat) minHeight = qMax(minHeight, fr.height);
  foreach(FormatRange fr, senderFormat) minHeight = qMax(minHeight, fr.height);

  words.clear();
  charPos.resize(textFormatted.text.length() + 1);
  charHeights.resize(textFormatted.text.length());
  charUrlIdx.fill(-1, textFormatted.text.length());
  for(int i = 0; i < textFormatted.urls.count(); i++) {
    Style::UrlInfo url = textFormatted.urls[i];
    for(int j = url.start; j < url.end; j++) charUrlIdx[j] = i;
  }
  if(!textFormat.count()) return;
  int idx = 0; int cnt = 0; int w = 0;
  QFontMetrics metrics(textFormat[0].format.font());
  Word wr;
  wr.start = -1; wr.trailing = -1;
  for(int i = 0; i < textFormatted.text.length(); ) {
    charPos[i] = w; charHeights[i] = textFormat[idx].height;
    w += metrics.charWidth(textFormatted.text, i);
    if(!textFormatted.text[i].isSpace()) {
      if(wr.trailing >= 0) {
        // new word after space
        words.append(wr);
        wr.start = -1;
      }
      if(wr.start < 0) {
        wr.start = i; wr.length = 1; wr.trailing = -1; wr.height = textFormat[idx].height;
      } else {
        wr.length++; wr.height = qMax(wr.height, textFormat[idx].height);
      }
    } else {
      if(wr.start < 0) {
        wr.start = i; wr.length = 0; wr.trailing = 1; wr.height = 0;
      } else {
        wr.trailing++;
      }
    }
    if(++i < textFormatted.text.length() && ++cnt >= textFormat[idx].length) {
      cnt = 0; idx++;
      Q_ASSERT(idx < textFormat.count());
      metrics = QFontMetrics(textFormat[idx].format.font());
    }
  }
  charPos[textFormatted.text.length()] = w;
  if(wr.start >= 0) words.append(wr);
}

qreal ChatLine::layout(qreal tsw, qreal senderw, qreal textw) {
  tsWidth = tsw; senderWidth = senderw; textWidth = textw;
  if(textw <= 0) return minHeight;
  lineLayouts.clear(); LineLayout line;
  int h = 0;
  int offset = 0; int numWords = 0;
  line.y = 0;
  line.start = 0;
  line.height = minHeight;  // first line needs room for ts and sender
  for(int i = 0; i < words.count(); i++) {
    int lastpos = charPos[words[i].start + words[i].length]; // We use charPos[lastchar + 1], 'coz last char needs to fit
    if(lastpos - offset <= textw) {
      line.height = qMax(line.height, words[i].height);
      line.length = words[i].start + words[i].length - line.start;
      numWords++;
    } else {
      // we need to wrap!
      if(numWords > 0) {
        // ok, we had some words before, so store the layout and start a new line
        h += line.height;
        line.length = words[i-1].start + words[i-1].length - line.start;
        lineLayouts.append(line);
        line.y += line.height;
        line.start = words[i].start;
        line.height = words[i].height;
        offset = charPos[words[i].start];
      }
      numWords = 1;
      // check if the word fits into the current line
      if(lastpos - offset <= textw) {
        line.length = words[i].length;
      } else {
        // we need to break a word in the middle
        int border = (int)textw + offset; // save some additions
        line.start = words[i].start;
        line.length = 1;
        line.height = charHeights[line.start];
        int j = line.start + 1;
        for(int l = 1; l < words[i].length; j++, l++) {
          if(charPos[j+1] < border) {
            line.length++;
            line.height = qMax(line.height, charHeights[j]);
            continue;
          } else {
            h += line.height;
            lineLayouts.append(line);
            line.y += line.height;
            line.start = j;
            line.height = charHeights[j];
            line.length = 1;
            offset = charPos[j];
            border = (int)textw + offset;
          }
        }
      }
    }
  }
  h += line.height;
  if(numWords > 0) {
    lineLayouts.append(line);
  }
  hght = h;
  return hght;
}

//!\brief Draw ChatLine on the given QPainter at the given position.
void ChatLine::draw(QPainter *p, const QPointF &pos) {
  QPalette pal = QApplication::palette();

  if(selectionMode == Full) {
    p->setPen(Qt::NoPen);
    p->setBrush(pal.brush(QPalette::Highlight));
    p->drawRect(QRectF(pos, QSizeF(tsWidth + Style::sepTsSender() + senderWidth + Style::sepSenderText() + textWidth, height())));
  } else if(selectionMode == Partial) {

  } /*
  p->setClipRect(QRectF(pos, QSizeF(tsWidth, height())));
  tsLayout.draw(p, pos, tsFormat);
  p->setClipRect(QRectF(pos + QPointF(tsWidth + Style::sepTsSender(), 0), QSizeF(senderWidth, height())));
  senderLayout.draw(p, pos + QPointF(tsWidth + Style::sepTsSender(), 0), senderFormat);
  p->setClipping(false);
  textLayout.draw(p, pos + QPointF(tsWidth + Style::sepTsSender() + senderWidth + Style::sepSenderText(), 0), textFormat);
    */
  //p->setClipRect(QRectF(pos, QSizeF(tsWidth, 15)));
  //p->drawRect(QRectF(pos, QSizeF(tsWidth, minHeight)));
  p->setBackgroundMode(Qt::OpaqueMode);
  QPointF tp = pos;
  QRectF rect(pos, QSizeF(tsWidth, minHeight));
  QRectF brect;
  foreach(FormatRange fr, tsFormat) {
    p->setFont(fr.format.font());
    p->setPen(QPen(fr.format.foreground(), 0)); p->setBackground(fr.format.background());
    p->drawText(rect, Qt::AlignLeft|Qt::TextSingleLine, tsFormatted.text.mid(fr.start, fr.length), &brect);
    rect.setLeft(brect.right());
  }
  rect = QRectF(pos + QPointF(tsWidth + Style::sepTsSender(), 0), QSizeF(senderWidth, minHeight));
  for(int i = senderFormat.count() - 1; i >= 0; i--) {
    FormatRange fr = senderFormat[i];
    p->setFont(fr.format.font()); p->setPen(QPen(fr.format.foreground(), 0)); p->setBackground(fr.format.background());
    p->drawText(rect, Qt::AlignRight|Qt::TextSingleLine, senderFormatted.text.mid(fr.start, fr.length), &brect);
    rect.setRight(brect.left());
  }
  QPointF tpos = pos + QPointF(tsWidth + Style::sepTsSender() + senderWidth + Style::sepSenderText(), 0);
  qreal h = 0; int l = 0;
  rect = QRectF(tpos + QPointF(0, h), QSizeF(textWidth, lineLayouts[l].height));
  int offset = 0;
  foreach(FormatRange fr, textFormat) {
    if(l >= lineLayouts.count()) break;
    p->setFont(fr.format.font()); p->setPen(QPen(fr.format.foreground(), 0)); p->setBackground(fr.format.background());
    int start, end, frend, llend;
    do {
      frend = fr.start + fr.length;
      if(frend <= lineLayouts[l].start) break;
      llend = lineLayouts[l].start + lineLayouts[l].length;
      start = qMax(fr.start, lineLayouts[l].start); end = qMin(frend, llend);
      rect.setLeft(tpos.x() + charPos[start] - offset);
      p->drawText(rect, Qt::AlignLeft|Qt::TextSingleLine, textFormatted.text.mid(start, end - start), &brect);
      if(llend <= end) {
        h += lineLayouts[l].height;
        l++;
        if(l < lineLayouts.count()) {
          rect = QRectF(tpos + QPointF(0, h), QSizeF(textWidth, lineLayouts[l].height));
          offset = charPos[lineLayouts[l].start];
        }
      }
    } while(end < frend && l < lineLayouts.count());
  }
}
