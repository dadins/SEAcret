#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include "AES.h"
#include "md5.h"

// return i in [a, b]
int bound(short i,short a,short b)
{
    return min(max(i,min(a,b)),max(a,b));
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->action_Open,SIGNAL(triggered()),this,SLOT(load()));
    connect(ui->action_Save,SIGNAL(triggered()),this,SLOT(save()));
    connect(ui->encryptBtn,SIGNAL(clicked()),this,SLOT(encrypt()));
    connect(ui->decryptBtn,SIGNAL(clicked()),this,SLOT(decrypt()));
    connect(ui->actionChange_Median_Blur_Kernel,SIGNAL(triggered()),this,SLOT(setKernel()));
    connect(ui->actionChange_Eraser_Size,SIGNAL(triggered()),this,SLOT(setR()));

    /* initialization */
    setMode(2); // Lasso
    setStyle(1); // median blur
    kernel = 51;
    setR(10);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::load()
{
    QString imageName = QFileDialog::getOpenFileName(this,"Open Image", "", "Image Files (*.jpg *.png)");
    if (imageName == NULL)
        return;
    filename = String(imageName.toAscii().data());
    destroyAllWindows();
    _src = imread(filename);
    if (!_src.data) {
        QMessageBox::critical(0, "Error", "No image file is loaded!!");
        return;
    }
    namedWindow("Image", CV_WINDOW_AUTOSIZE);
    imshow("Image", _src);
    _dst.release();
    ui->encryptBtn->setEnabled(true);
    ui->decryptBtn->setEnabled(true);
}

void MainWindow::save()
{
    if (!_dst.data) {
        QMessageBox::critical(0, "Error", "There is nothing to be saved");
        return;
    }

    QString imageName = QFileDialog::getSaveFileName(this,"Save Image", "", "Image Files (*.jpg)");
    if (imageName == NULL)
        return;

    imwrite(imageName.toAscii().data(), _dst);

    if (isEncryption) {
        AES aes(key);

        int buf_size[2];

        vector<unsigned char> mask_buf, secret_buf;
        unsigned char *temp;

        vector<int> p;
        p.push_back(CV_IMWRITE_JPEG_QUALITY);
        p.push_back(50);
        imencode(".jpg", zone, mask_buf, p);

        for (int i = (((mask_buf.size()+15)>>4)<<4) - mask_buf.size(); i > 0; i--)
            mask_buf.push_back(0);

        for (unsigned int i = 0; i < mask_buf.size(); i+=16) {
            temp = &mask_buf[0]+i;
            aes.Cipher(temp);
        }

        Mat secret;
        dilate(zone, zone, Mat(7,7,CV_8U,Scalar(1)));
        _src.copyTo(secret, zone);
        imencode(".jpg", secret, secret_buf);

        for (int i = (((secret_buf.size()+15)>>4)<<4) - secret_buf.size(); i > 0; i--)
            secret_buf.push_back(0);

        for (unsigned int i = 0; i < secret_buf.size(); i+=16) {
            temp = &secret_buf[0]+i;
            aes.Cipher(temp);
        }

        unsigned char info[16] = "";
        buf_size[0] = mask_buf.size();
        buf_size[1] = secret_buf.size();
        memcpy(info, buf_size, sizeof(int)*2);
        memcpy(info+8, "SEAcret", 7);
        aes.Cipher(info);

        // append encrypted block of image and encryted info to blurred image
        FILE *f = fopen(imageName.toAscii().data(), "ab");
        fwrite(&mask_buf[0], 1, mask_buf.size(), f);
        fwrite(&secret_buf[0], 1, secret_buf.size(), f);
        fwrite(info, 1, 16, f);
        fclose(f);
    }
}

void MainWindow::encrypt()
{
    isEncryption = true;

    QString text = QInputDialog::getText(this, "Password",
                     "Please enter your password for encryption",
                      QLineEdit::Password, QString());

    string pwd = string((const char *)text.toLocal8Bit());
    MD5 md5(pwd);
    key = md5.getDigest();

    _dst = _src.clone();
    zone = Mat::zeros(_src.size(), CV_8UC1);

    setMouseCallback("Image", &mouseHandler, this);
    imshow("Image", _dst);
    waitKey(0);
    destroyAllWindows();
}

void MainWindow::decrypt()
{
    FILE *f = fopen(filename.c_str(), "rb");
    if (!f) {
        QMessageBox::critical(0, "Error", "No image file is loaded!!");
        return;
    }

    isEncryption = false;

    QString text = QInputDialog::getText(this, "Password",
                     "Please enter your password for decryption",
                      QLineEdit::Password, QString());

    string pwd = string((const char *)text.toLocal8Bit());
    MD5 md5(pwd);
    key = md5.getDigest();

    AES aes(key);

    unsigned char info[16] = "";
    fseek(f, -16, SEEK_END);
    fread(info, 1, 16, f);

    // verify key and info
    aes.InvCipher(info);
    if (memcmp(info+8, "SEAcret", 7)) {
        QMessageBox::critical(0, "Error", "Incorrect password or there is no secret in the image");
        return;
    }

    int buf_size[2];
    memcpy(buf_size, info, sizeof(int)*2);
    fseek(f, -16-buf_size[0]-buf_size[1], SEEK_END);
    unsigned char *temp;

    vector<unsigned char> mask_buf(buf_size[0]);
    vector<unsigned char> secret_buf(buf_size[1]);
    fread(&mask_buf[0], 1, buf_size[0], f);
    fread(&secret_buf[0], 1, buf_size[1], f);
    fclose(f);

    for (int i = 0; i < buf_size[0]; i+=16) {
        temp = &mask_buf[0]+i;
        aes.InvCipher(temp);
    }

    Mat mask = imdecode((Mat)mask_buf, CV_LOAD_IMAGE_GRAYSCALE);
    mask = mask > 128;

    for (int i = 0; i < buf_size[1]; i+=16) {
        temp = &secret_buf[0]+i;
        aes.InvCipher(temp);
    }

    Mat secret = imdecode((Mat)secret_buf, 1);

    _dst = imread(filename, 1);
    secret.copyTo(_dst, mask);

    imshow("Result", _dst);
    waitKey(0);
    destroyAllWindows();
}

void MainWindow::mouseHandler(int event, int x, int y, int flags, void* param)
{
    MainWindow* window = (MainWindow*) param;
    if (window->mode == 3) {
        x = bound(x, window->R, window->_src.cols-1-window->R);
        y = bound(y, window->R, window->_src.rows-1-window->R);
    }
    else {
        x = bound(x, 0, window->_src.cols-1);
        y = bound(y, 0, window->_src.rows-1);
    }

    // user press left button
    if (event == CV_EVENT_LBUTTONDOWN && !window->drag) {
        if (window->mode == 1)
            window->point = Point(x, y);
        else if (window->mode == 2) {
            window->points.clear();
            window->points.push_back(Point(x, y));
        }
        else if (window->mode == 3) {
            Rect box = Rect(x-window->R, y-window->R, 1+window->R*2, 1+window->R*2);
            Mat temp;
            medianBlur(window->_dst(box), temp, window->kernel);
            temp.copyTo(window->_dst(box), window->round_mask);
            window->round_mask.copyTo(window->zone(box), window->round_mask);
            imshow("Image", window->_dst);
        }
        window->drag = true;
    }

    // user drag the mouse
    if (event == CV_EVENT_MOUSEMOVE && window->drag) {
        Mat temp = window->_dst.clone();

        if (window->mode == 1)
            rectangle(temp, window->point, Point(x, y), CV_RGB(255, 0, 0), 2, 8, 0);
        else if (window->mode == 2) {
            window->pts = (const Point*)Mat(window->points).data;
            window->npts = window->points.size();
            window->points.push_back(Point(x, y));
            polylines(temp, &window->pts, &window->npts, 1, false, Scalar(0,0,255), 2, 8, 0);
        }
        else if (window->mode == 3) {
            Rect box = Rect(x-window->R, y-window->R, 1+window->R*2, 1+window->R*2);
            Mat temp;
            medianBlur(window->_dst(box), temp, window->kernel);
            temp.copyTo(window->_dst(box), window->round_mask);
            window->round_mask.copyTo(window->zone(box), window->round_mask);
            imshow("Image", window->_dst);
            return;
        }

        imshow("Image", temp);
    }

    // user release left button
    if (event == CV_EVENT_LBUTTONUP && window->drag) {
        if (window->mode == 1) {
            Point temp(window->point);
            window->point.x = min(x, temp.x);
            x = max(x, temp.x);
            window->point.y = min(y, temp.y);
            y = max(y, temp.y);

            rectangle(window->zone, window->point, Point(x, y), Scalar(255), CV_FILLED, 8, 0);
            Rect box = Rect(window->point.x, window->point.y, x - window->point.x, y - window->point.y);

            if (window->style == 1) {
                medianBlur(window->_dst(box), window->_dst(box), window->kernel);
                medianBlur(window->_dst(box), window->_dst(box), window->kernel);
            }
            else {
                Mat mask = Mat::zeros(window->_src.size(), CV_8UC1);
                rectangle(mask, window->point, Point(x, y), CV_RGB(255, 255, 255), CV_FILLED, 8, 0);
                inpaint(window->_dst, mask, window->_dst, 5, INPAINT_TELEA);
            }
        }
        else if (window->mode == 2) {
            Rect box = boundingRect(window->points);
            vector< vector<Point> > contour;
            contour.push_back(window->points);
            Mat mask = Mat::zeros(window->_src.size(), CV_8UC1);
            fillPoly(mask, contour, Scalar(255));
            fillPoly(window->zone, contour, Scalar(255));

            if (window->style == 1) {
                Mat temp;
                medianBlur(window->_dst(box), temp, window->kernel);
                medianBlur(temp, temp, window->kernel);
                temp.copyTo(window->_dst(box), mask(box));
            }
            else
                inpaint(window->_dst, mask, window->_dst, 5, INPAINT_TELEA);
        }

        imshow("Image", window->_dst);
        window->drag = false;
    }

    // user click right button: reset all
    if (event == CV_EVENT_RBUTTONUP) {
        window->_dst = window->_src.clone();
        window->zone = Mat::zeros(window->_src.size(), CV_8UC1);
        window->drag = false;
        imshow("Image", window->_dst);
    }
}

void MainWindow::setMode(int m)
{
    mode = m;
    drag = false;
}

void MainWindow::setStyle(int s)
{
    style = s;
    drag = false;
}

void MainWindow::setKernel()
{
    int input = QInputDialog::getInteger(this, "Medain Blur Kernel Size", "Enter an integer (1~49)", 25, 1, 49, 1);
    kernel = 1 + input*2;
}

void MainWindow::setR(int r)
{
    if (r)
        R = r;
    else {
        int input = QInputDialog::getInteger(this, "Eraser Size", "Enter an integer (1~49)", 10, 1, 49, 1);
        R = input;
    }
    round_mask = Mat::zeros(Size(1+R*2, 1+R*2), CV_8UC1);
    circle(round_mask, Point(R,R), R, Scalar(255), CV_FILLED, 8, 0);
}

void MainWindow::on_actionRectangle_triggered()
{
    setMode(1);
}

void MainWindow::on_actionLasso_triggered()
{
    setMode(2);
}

void MainWindow::on_actionEraser_triggered()
{
    setMode(3);
}

void MainWindow::on_actionMedian_Blur_triggered()
{
    setStyle(1);
}

void MainWindow::on_actionInpaint_triggered()
{
    setStyle(2);
}
