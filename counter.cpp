#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QPen>
#include <QBrush>
#include <QWindow>
#include <QTimer>
#include <QDesktopWidget>
#include <QGraphicsDropShadowEffect>
#include <QDir>
#include <QPainterPath>
#include <QPainter>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>

#include <sstream>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <unistd.h>
#include <sys/time.h>
#include <cmath>

#include "embed.cpp"

double now()
{
  timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}
void reposition();

class OutlineLabel: public QLabel
{
public:
  OutlineLabel()
  {
    this->w = 1.0 / 25.0;
    this->brush = QBrush(Qt::white);
    this->pen = QPen(Qt::black);
  }
  QBrush brush;
  QPen pen;
  double w;
  bool defaultMode = false;
  double outlineThickness() const
  {
    return w * font().pointSize();
  }
  QSize sizeHint() const override
  {
    auto w = std::ceil(outlineThickness() * 2.0);
    return QLabel::sizeHint() + QSize(w, w);
  }
  QSize minimumSizeHint() const override
  {
    auto w = std::ceil(outlineThickness() * 2.0);
    return QLabel::minimumSizeHint() + QSize(w, w);
  }
  void paintEvent(QPaintEvent* event) override
  {
    if (defaultMode)
    {
      QLabel::paintEvent(event);
      return;
    }
    auto w = outlineThickness();
    auto rect = this->rect();
    auto metrics = QFontMetrics(font());
    auto tr = metrics.boundingRect(text()).adjusted(0, 0, w, w);
    double ind;
    if (indent() == -1)
    {
      if (frameWidth())
        ind = (metrics.boundingRect('x').width() + w * 2) / 2;
      else
        ind = w;
    }
    else
      ind = indent();
    double x;
    double y;
    if (alignment() & Qt::AlignLeft)
      x = rect.left() + ind - std::min(metrics.leftBearing(text()[0]), 0);
    else if (alignment() & Qt::AlignRight)
      x = rect.x() + rect.width() - ind - tr.width();
    else
      x = (rect.width() - tr.width()) / 2.0;
            
    if (alignment() & Qt::AlignTop)
      y = rect.top() + ind + metrics.ascent();
    else if (alignment() & Qt::AlignBottom)
      y = rect.y() + rect.height() - ind - metrics.descent();
    else
      y = (rect.height() + metrics.ascent() - metrics.descent()) / 2.0;

    auto path = QPainterPath();
    path.addText(x, y, font(), text());
    QPainter qp(this);
    qp.setRenderHint(QPainter::Antialiasing);

    pen.setWidthF(w * 2);
    qp.strokePath(path, pen);
    if (1 < brush.style() && brush.style() < 15)
      qp.fillPath(path, palette().window());
    qp.fillPath(path, brush);
  }
};

QPixmap* pix;
QMainWindow* mw;
OutlineLabel* label;
bool running = true;
int duration = 180;
double start;
int px = 10;
int py = 10;
int wwidth;
int wheight;
bool inPopup = false;

class MouseClickEventFilter : public QObject {

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        // Check if the event is a mouse button press
        if (event->type() == QEvent::MouseButtonPress && !inPopup) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            
            // Check for left mouse button
            if (mouseEvent->button() == Qt::LeftButton) {
              if (!running)
              {
                running = true;
                start = now();
              }
              else
              {
                if (px < 500)
                  if (py < 500)
                    px = -1;
                  else
                    py = 1;
                else
                  if (py < 500)
                    py = -1;
                  else
                    px = 1;
                QTimer::singleShot(std::chrono::milliseconds(200), &reposition);
              }
                return true; // Event handled
            }
        }
        // Pass unhandled events to the base class
        return QObject::eventFilter(obj, event);
    }
};

class MainWindow: public QMainWindow
{
public:
  void contextMenuEvent(QContextMenuEvent *event) override {
        QMenu contextMenu(this);

        // Add actions to the menu
        QAction *action1 = contextMenu.addAction("Reset");
        QAction *action2 = contextMenu.addAction("Reset and start");
        QAction *action3 = contextMenu.addAction("Quit");

        // Show the context menu at the cursor's position
        inPopup = true;
        QAction *selectedAction = contextMenu.exec(event->globalPos());
        inPopup = false;
        // Handle the selected action
        if (selectedAction == action1) {
          running = false;
          QTimer::singleShot(std::chrono::milliseconds(200), &reposition);
        } else if (selectedAction == action2) {
          running = true;
          start = now();
          QTimer::singleShot(std::chrono::milliseconds(200), &reposition);
        } else if (selectedAction == action3) {
           QCoreApplication::quit();
        }
  }
  QMenu* createPopupMenu() override
  {
    auto res = new QMenu(this);
    auto a = new QAction("Reset", res);
    res->insertAction(nullptr, a);
    return res;
  }
};

void fire()
{
  double elapsed = now()-start;
  double remain = duration - elapsed;
  if (!running)
    remain = duration;
  if (remain < 0)
  {
    label->defaultMode = true;
    label->setPixmap(*pix);
    QTimer::singleShot(std::chrono::milliseconds(50), &fire);
    return;
  }
  label->defaultMode = false;
  int minutes = (int)remain / 60;
  int seconds = ((int)remain) % 60;
  std::stringstream st;
  st << minutes << ':' << ((seconds < 10) ? "0": "") << seconds;
  label->setText(QString(st.str().c_str()));
  QTimer::singleShot(std::chrono::milliseconds(50), &fire);
}

void reposition()
{
  if (px < 0)
    px = wwidth - mw->width() + px;
  if (py < 0)
    py = wheight - mw->height() + py;
  mw->adjustSize();
  mw->windowHandle()->setPosition(px, py);
}

int main(int argc, char **argv)
{
  QApplication app(argc, argv);
  auto const rec = QApplication::desktop()->screenGeometry();
  wheight = rec.height();
  wwidth = rec.width();
  
  pix = new QPixmap();
  pix->loadFromData(embeded_files["tadaa.png"].data(), embeded_files["tadaa.png"].size());
  int fontsize = 64;
  for (int i=1; i<argc; i++)
  {
    std::string a(argv[i]);
    if (a == "-s")
      fontsize = std::atoi(argv[++i]);
    else if (a == "-d")
      duration = std::atoi(argv[++i]);
    else if (a == "-p")
    {
      px = std::atoi(argv[++i]);
      py = std::atoi(argv[++i]);
    }
    else if (a == "-h" || a == "--help")
    {
      std::cout << "Usage: counter OPTIONS\n"
       << "\t-s FONT_SIZE\n"
       << "\t-d DURATION_SECONDS\n"
       << "\t-p PX PY\n"
       << std::endl;
       return 0;
    }
  }
  MouseClickEventFilter *filter = new MouseClickEventFilter();
  app.installEventFilter(filter);
  mw = new MainWindow();
  mw->setStyleSheet("background:transparent");
  mw->setAttribute(Qt::WA_TranslucentBackground);
  mw->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
  auto* l = new OutlineLabel();
  label = l;
  QFont font = label->font();
  font.setPointSize(fontsize);  // Set font size to 16
  label->setFont(font);
  mw->setCentralWidget(l);
  mw->show();
  if (px >= 0 && py >= 0)
    mw->windowHandle()->setPosition(px, py);
  else
     QTimer::singleShot(std::chrono::milliseconds(200), &reposition);
  QTimer::singleShot(std::chrono::milliseconds(50), &fire);
  start = now();
  app.exec();
}