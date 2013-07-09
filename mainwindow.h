#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <opencv2/opencv.hpp>
using namespace cv;
namespace Ui {
class MainWindow;
}
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    
private:
    Ui::MainWindow *ui;
    Mat _src;	// src img
    Mat _dst;	// dst img
    Mat zone;   // encrypted zone

    Point point; // store cursor position
    bool drag; // if user is dragging
    int mode; // 1: rectangle; 2: Lasso; 3: eraser
    int style; // 1: median blur; 2: inpaint
    int kernel; // median blur kernel size

    /* used for eraser tool */
    Mat round_mask;
    int R;

    /* used for inpaint*/
    vector<Point> points;
    const Point *pts;
    int npts;

    unsigned char *key;
    String filename;
    bool isEncryption;

    void setMode(int);
    void setStyle(int);
    static void mouseHandler(int, int, int, int, void*);

private slots:
    void load();	// load
    void save();	// save
    void encrypt();	// encrypttion
    void decrypt(); // decrypttion
    void on_actionRectangle_triggered();
    void on_actionLasso_triggered();
    void on_actionInpaint_triggered();
    void on_actionMedian_Blur_triggered();
    void setKernel();
    void setR(int = 0);
    void on_actionEraser_triggered();
};
#endif // MAINWINDOW_H
