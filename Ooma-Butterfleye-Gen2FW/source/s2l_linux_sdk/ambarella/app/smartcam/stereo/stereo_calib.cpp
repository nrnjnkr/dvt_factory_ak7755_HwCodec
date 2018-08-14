 /*
 * History:
 *	2016/12/30 - [JianTang] Created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "cv.h"
#include "highgui.h"
#include "iostream"
#include <vector>
#include <stdio.h>
#include <sys/time.h>
#include <getopt.h>
#include "iav.h"
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

using namespace std;
using namespace	cv;

#define NX					9
#define NY					6

#define	LEFT_IAMGE			0
#define	RIGHT_IAMGE			1

#define MULTIPLE			10

struct option long_options[] = {
	{"valid_frames",	1,	0,	'n'},
	{"path",			1,	0,	'p'},
	{0,					0,	0,	 0 },
};

const char *short_options = "n:p:";

static int	g_valid_frames = 10;
static char	g_path[64] = "/sdcard/stereo/calib";

int parse_parameters(int argc, char **argv)
{
	int		c;

	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {
		case 'n':
			g_valid_frames = atoi(optarg);
			break;

		case 'p':
			strcpy(g_path, optarg);
			break;

		default:
			printf("Unknown parameter %c!\n", c);
			break;

		}
	}

	return 0;
}

void _initUndistortRectifyMap( InputArray _cameraMatrix, InputArray _distCoeffs,
                              InputArray _matR, InputArray _newCameraMatrix,
                              Size size, int m1type, OutputArray _map1, OutputArray _map2 )
{
    Mat cameraMatrix = _cameraMatrix.getMat(), distCoeffs = _distCoeffs.getMat();
    Mat matR = _matR.getMat(), newCameraMatrix = _newCameraMatrix.getMat();

    if( m1type <= 0 )
        m1type = CV_16SC2;
    CV_Assert( m1type == CV_16SC2 || m1type == CV_32FC1 || m1type == CV_32FC2 );
    _map1.create( size, m1type );
    Mat map1 = _map1.getMat(), map2;
    if( m1type != CV_32FC2 )
    {
        _map2.create( size, m1type == CV_16SC2 ? CV_16UC1 : CV_32FC1 );
        map2 = _map2.getMat();
    }
    else
        _map2.release();

    Mat_<double> R = Mat_<double>::eye(3, 3);
    Mat_<double> A = Mat_<double>(cameraMatrix), Ar;

    if( newCameraMatrix.data )
        Ar = Mat_<double>(newCameraMatrix);
    else
        Ar = getDefaultNewCameraMatrix( A, size, true );

    if( matR.data )
        R = Mat_<double>(matR);

    if( distCoeffs.data )
        distCoeffs = Mat_<double>(distCoeffs);
    else
    {
        distCoeffs.create(8, 1, CV_64F);
        distCoeffs = 0.;
    }

    CV_Assert( A.size() == Size(3,3) && A.size() == R.size() );
    CV_Assert( Ar.size() == Size(3,3) || Ar.size() == Size(4, 3));
    Mat_<double> iR = (Ar.colRange(0,3)*R).inv(DECOMP_LU);
    const double* ir = &iR(0,0);

    double u0 = A(0, 2),  v0 = A(1, 2);
    double fx = A(0, 0),  fy = A(1, 1);

    CV_Assert( distCoeffs.size() == Size(1, 4) || distCoeffs.size() == Size(4, 1) ||
               distCoeffs.size() == Size(1, 5) || distCoeffs.size() == Size(5, 1) ||
               distCoeffs.size() == Size(1, 8) || distCoeffs.size() == Size(8, 1));

    if( distCoeffs.rows != 1 && !distCoeffs.isContinuous() )
        distCoeffs = distCoeffs.t();

    double k1 = ((double*)distCoeffs.data)[0];
    double k2 = ((double*)distCoeffs.data)[1];
    double p1 = ((double*)distCoeffs.data)[2];
    double p2 = ((double*)distCoeffs.data)[3];
    double k3 = distCoeffs.cols + distCoeffs.rows - 1 >= 5 ? ((double*)distCoeffs.data)[4] : 0.;
    double k4 = distCoeffs.cols + distCoeffs.rows - 1 >= 8 ? ((double*)distCoeffs.data)[5] : 0.;
    double k5 = distCoeffs.cols + distCoeffs.rows - 1 >= 8 ? ((double*)distCoeffs.data)[6] : 0.;
    double k6 = distCoeffs.cols + distCoeffs.rows - 1 >= 8 ? ((double*)distCoeffs.data)[7] : 0.;

    for( int i = 0; i < size.height; i++ )
    {
        float* m1f = (float*)(map1.data + map1.step*i);
        float* m2f = (float*)(map2.data + map2.step*i);
        short* m1 = (short*)m1f;
        ushort* m2 = (ushort*)m2f;
        double _x = i*ir[1]/MULTIPLE + ir[2], _y = i*ir[4]/MULTIPLE + ir[5], _w = i*ir[7]/MULTIPLE + ir[8];

        for( int j = 0; j < size.width; j++, _x += ir[0]/MULTIPLE, _y += ir[3]/MULTIPLE, _w += ir[6]/MULTIPLE )
        {
            double w = 1./_w, x = _x*w, y = _y*w;
            double x2 = x*x, y2 = y*y;
            double r2 = x2 + y2, _2xy = 2*x*y;
            double kr = (1 + ((k3*r2 + k2)*r2 + k1)*r2)/(1 + ((k6*r2 + k5)*r2 + k4)*r2);
            double u = fx*(x*kr + p1*_2xy + p2*(r2 + 2*x2)) + u0;
            double v = fy*(y*kr + p1*(r2 + 2*y2) + p2*_2xy) + v0;
            if( m1type == CV_16SC2 )
            {
                int iu = saturate_cast<int>(u*INTER_TAB_SIZE);
                int iv = saturate_cast<int>(v*INTER_TAB_SIZE);
                m1[j*2] = (short)(iu >> INTER_BITS);
                m1[j*2+1] = (short)(iv >> INTER_BITS);
                m2[j] = (ushort)((iv & (INTER_TAB_SIZE-1))*INTER_TAB_SIZE + (iu & (INTER_TAB_SIZE-1)));
            }
            else if( m1type == CV_32FC1 )
            {
                m1f[j] = (float)u;
                m2f[j] = (float)v;
            }
            else
            {
                m1f[j*2] = (float)u;
                m1f[j*2+1] = (float)v;
            }
        }
    }
}

void _cvInitUndistortRectifyMap( const CvMat* Aarr, const CvMat* dist_coeffs,
    const CvMat *Rarr, const CvMat* ArArr, CvArr* mapxarr, CvArr* mapyarr )
{
    cv::Mat A = cv::cvarrToMat(Aarr), distCoeffs, R, Ar;
    cv::Mat mapx = cv::cvarrToMat(mapxarr), mapy, mapx0 = mapx, mapy0;

    if( mapyarr )
        mapy0 = mapy = cv::cvarrToMat(mapyarr);

    if( dist_coeffs )
        distCoeffs = cv::cvarrToMat(dist_coeffs);
    if( Rarr )
        R = cv::cvarrToMat(Rarr);
    if( ArArr )
        Ar = cv::cvarrToMat(ArArr);

    _initUndistortRectifyMap( A, distCoeffs, R, Ar, mapx.size(), mapx.type(), mapx, mapy );
    CV_Assert( mapx0.data == mapx.data && mapy0.data == mapy.data );
}

int main(int argc, char **argv)
{
	const float					square_size = 2.54;
	unsigned int				i, j, vf, corners = NX * NY, n = 0;
	vector<CvPoint2D32f>		points[2];
	vector<CvPoint2D32f>		templ(corners), tempr(corners);
	vector<CvPoint3D32f>		objectPoints;
	vector<int>					npoints;
	unsigned int				p, w, h;
	int							cntl, cntr;
	int							retl, retr;
	char						file[256];

	IplImage					*imgl, *imgr;
	vector<CvPoint2D32f>		&ptsl = points[LEFT_IAMGE];
	vector<CvPoint2D32f>		&ptsr = points[RIGHT_IAMGE];

	struct timeval				tv_begin, tv_end;
	int							seconds;

	double						m1[3][3], m2[3][3], d1[5], d2[5];
	double						r[3][3], t[3], e[3][3], f[3][3];

	CvMat						M1 = cvMat(3, 3, CV_64F, m1);
	CvMat						M2 = cvMat(3, 3, CV_64F, m2);
	CvMat						D1 = cvMat(1, 5, CV_64F, d1);
	CvMat						D2 = cvMat(1, 5, CV_64F, d2);
	CvMat						R  = cvMat(3, 3, CV_64F, r);
	CvMat						T  = cvMat(3, 1, CV_64F, t);
	CvMat						E  = cvMat(3, 3, CV_64F, e);
	CvMat						F  = cvMat(3, 3, CV_64F, f);

	int							ret;
	char						*fp;

	ret = parse_parameters(argc, argv);
	if (ret < 0) {
		printf("ERROR: Incorrect parameters!\n");
		return -1;
	}

	ret = init_iav(IAV_BUF_MAIN, IAV_BUF_ME1);
	if (ret < 0) {
		printf("ERROR: Unable to init iav!\n");
		return -1;
	}

	ret = get_iav_buf_size(&p, &w, &h);
	if (ret < 0 || w != 480 * 2 || h != 270) {
		printf("Error: Incorrect iav buffer!\n");
		return -1;
	}

	w = 480;

	gettimeofday(&tv_begin, NULL);

	cvSetIdentity(&M1);
	cvSetIdentity(&M2);
	cvZero(&D1);
	cvZero(&D2);

	imgl = cvCreateImage(cvSize(w, h),IPL_DEPTH_8U,1);
	imgr = cvCreateImage(cvSize(w, h),IPL_DEPTH_8U,1);

	/* Step 1: Find All Corners */
	vf = 0;
	while (vf < (unsigned int)g_valid_frames) {
		printf("Capturing chessboard pictures (%d/%d) ...", vf + 1, g_valid_frames);
		getchar();

		fp = get_iav_buf();
		if (!fp) {
			printf("ERROR: Unable to read iav buffer!\n");
			return -1;
		}

		for (j = 0; j < h; j++) {
			memcpy(imgl->imageData + j * imgl->widthStep, fp + j * p, w);
			memcpy(imgr->imageData + j * imgr->widthStep, fp + j * p + w, w);
		}

		cntl = 0;
		cntr = 0;
		retl = cvFindChessboardCorners(imgl, cvSize(NX, NY), &templ[0], &cntl, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE);
		retr = cvFindChessboardCorners(imgr, cvSize(NX, NY), &tempr[0], &cntr, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE);
		if (retl && retr && cntl == (int)corners && cntr == (int)corners) {
			cvFindCornerSubPix(imgl, &templ[0], cntl, cvSize(5, 5), cvSize(-1, -1), cvTermCriteria(CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 30, 0.01));
			n = ptsl.size();
		    ptsl.resize(n + corners, cvPoint2D32f(0, 0));
	        copy(templ.begin(), templ.end(), ptsl.begin() + n);

			cvFindCornerSubPix(imgr, &tempr[0], cntr, cvSize(5, 5), cvSize(-1, -1), cvTermCriteria(CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 30, 0.01));
			n = ptsr.size();
		    ptsr.resize(n + corners, cvPoint2D32f(0, 0));
	        copy(tempr.begin(), tempr.end(), ptsr.begin() + n);

			vf++;
			sprintf(file, "%s/left%02d.jpg", g_path, vf);
			cvSaveImage(file, imgl);
			sprintf(file, "%s/right%02d.jpg", g_path, vf);
			cvSaveImage(file, imgr);
		} else {
			printf("WARNING: Invalid images: %s\n", file);
		}
	}
	printf("Caputure chessboard pictures done.\n\n");

	/* Step 2: Stereo Calibration */
	printf("=================== Single Calibration ====================\n");
	n = vf * corners;
	objectPoints.resize(n);
	for (i = 0; i < NY; i++) {
		for(j = 0; j < NX; j++) {
			objectPoints[i * NX + j] = cvPoint3D32f(i * square_size, j * square_size, 0);
	    }
	}
	for (i = 1; i < vf; i++) {
	    copy(objectPoints.begin(), objectPoints.begin() + corners, objectPoints.begin() + i * corners);
	}
	npoints.resize(vf, corners);
	CvMat oPoints	= cvMat(1, n, CV_32FC3, &objectPoints[0]);
	CvMat iPoints1	= cvMat(1, n, CV_32FC2, &points[ LEFT_IAMGE][0]);
	CvMat iPoints2	= cvMat(1, n, CV_32FC2, &points[RIGHT_IAMGE][0]);
	CvMat _npoints	= cvMat(1, npoints.size(), CV_32S, &npoints[0]);

	cvCalibrateCamera2(&oPoints, &iPoints1, &_npoints, cvSize(w, h), &M1, &D1, NULL, NULL, CV_CALIB_FIX_K2 | CV_CALIB_FIX_K3 | CV_CALIB_ZERO_TANGENT_DIST);
	cvCalibrateCamera2(&oPoints, &iPoints2, &_npoints, cvSize(w, h), &M2, &D2, NULL, NULL, CV_CALIB_FIX_K2 | CV_CALIB_FIX_K3 | CV_CALIB_ZERO_TANGENT_DIST);

	printf("M1:\n");
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			printf("%5.1f\t", m1[i][j]);
		}
		printf("\n");
	}
	printf("D1:\n");
	for (j = 0; j < 5; j++) {
		printf("%5.2f\t", d1[j]);
	}
	printf("\n\n");
	printf("M2:\n");
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			printf("%5.1f\t", m2[i][j]);
		}
		printf("\n");
	}
	printf("D2:\n");
	for (j = 0; j < 5; j++) {
		printf("%5.2f\t", d2[j]);
	}
	printf("\n\n");
	printf("=================== Stereo Calibration ====================\n");
	cvStereoCalibrate(&oPoints, &iPoints1, &iPoints2, &_npoints,
	    &M1, &D1, &M2, &D2, cvSize(w, h), &R, &T, &E, &F,
	   CV_CALIB_USE_INTRINSIC_GUESS | CV_CALIB_ZERO_TANGENT_DIST | CV_CALIB_FIX_K2 | CV_CALIB_FIX_K3,
	   cvTermCriteria(CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 100, 1e-5));
	printf("M1:\n");
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			printf("%5.1f\t", m1[i][j]);
		}
		printf("\n");
	}
	printf("D1:\n");
	for (j = 0; j < 5; j++) {
		printf("%5.2f\t", d1[j]);
	}
	printf("\n\n");
	printf("M2:\n");
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			printf("%5.1f\t", m2[i][j]);
		}
		printf("\n");
	}
	printf("D2:\n");
	for (j = 0; j < 5; j++) {
		printf("%5.2f\t", d2[j]);
	}
	printf("\n");

	vector<CvPoint3D32f> lines[2];
	points[ LEFT_IAMGE].resize(n);
	points[RIGHT_IAMGE].resize(n);
	lines[ LEFT_IAMGE].resize(n);
	lines[RIGHT_IAMGE].resize(n);

	CvMat L1 = cvMat(1, n, CV_32FC3, &lines[ LEFT_IAMGE][0]);
	CvMat L2 = cvMat(1, n, CV_32FC3, &lines[RIGHT_IAMGE][0]);

	cvUndistortPoints(&iPoints1, &iPoints1, &M1, &D1, 0, &M1);
	cvUndistortPoints(&iPoints2, &iPoints2, &M2, &D2, 0, &M2);
	cvComputeCorrespondEpilines(&iPoints1, 1, &F, &L1);
	cvComputeCorrespondEpilines(&iPoints2, 2, &F, &L2);

	double mean_err = 0;
	for (i = 0; i < n; i++) {
	    mean_err += fabs(points[ LEFT_IAMGE][i].x * lines[RIGHT_IAMGE][i].x + points[ LEFT_IAMGE][i].y * lines[RIGHT_IAMGE][i].y + lines[RIGHT_IAMGE][i].z);
		mean_err += fabs(points[RIGHT_IAMGE][i].x * lines[ LEFT_IAMGE][i].x + points[RIGHT_IAMGE][i].y * lines[ LEFT_IAMGE][i].y + lines[ LEFT_IAMGE][i].z);
	}
	printf("\nMean Error = %.2f\n", mean_err / n);

	CvMat	*mx1 = cvCreateMat(h, w, CV_32F);
	CvMat	*my1 = cvCreateMat(h, w, CV_32F);
	CvMat	*mx2 = cvCreateMat(h, w, CV_32F);
	CvMat	*my2 = cvCreateMat(h, w, CV_32F);
	CvMat	*MX1 = cvCreateMat(MULTIPLE * h, MULTIPLE * w, CV_32F);
	CvMat	*MY1 = cvCreateMat(MULTIPLE * h, MULTIPLE * w, CV_32F);
	CvMat	*imx = cvCreateMat(h, w, CV_32F);
	CvMat	*imy = cvCreateMat(h, w, CV_32F);
	CvMat	*imz = cvCreateMat(h, w, CV_32F);

	double r1[3][3], r2[3][3];
	CvMat R1 = cvMat(3, 3, CV_64F, r1);
	CvMat R2 = cvMat(3, 3, CV_64F, r2);

	double p1[3][4], p2[3][4];
	double q[4][4];

	CvMat P1 = cvMat(3, 4, CV_64F, p1);
	CvMat P2 = cvMat(3, 4, CV_64F, p2);
	CvMat Q  = cvMat(4, 4, CV_64F, q);

	cvStereoRectify(&M1, &M2, &D1, &D2, cvSize(w, h), &R, &T, &R1, &R2, &P1, &P2, &Q, CV_CALIB_ZERO_DISPARITY);
	printf("Q:\n");
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			printf("%5.1f\t", q[i][j]);
		}
		printf("\n");
	}

    cvInitUndistortRectifyMap(&M1, &D1, &R1, &P1, mx1, my1);
	cvInitUndistortRectifyMap(&M2, &D1, &R2, &P2, mx2, my2);

	printf("-T = %.2f   f = %.2f    -T * f = %.2f\n", 1.0 / q[3][2], q[2][3], q[2][3] / q[3][2]);

	float	x, y, z, w00, w01, w10, w11;
	unsigned short X, Y, W00, W01, W10, W11;
	unsigned int Z;

	CvMat	*left = cvCreateMat(h, 6 * w, CV_16UC1);
	for (i = 0; i < w; i++) {
		for (j = 0; j < h; j++) {
			x = CV_MAT_ELEM(*mx1, float, j, i);
			y = CV_MAT_ELEM(*my1, float, j, i);
			X = cvFloor(x);
			Y = cvFloor(y);
			Z = Y * w + X;
			x = x - X;
			y = y - Y;
			w00 = (1 - x) * (1 - y);
			w01 = x * (1 - y);
			w10 = (1 - x) * y;
			w11 = x * y;

			w00 *= 65536;
			if (w00 > 65535) {
				W00 = 65535;
			} else {
				W00 = w00;
			}

			w01 *= 65536;
			if (w01 > 65535) {
				W01 = 65535;
			} else {
				W01 = w01;
			}

			w10 *= 65536;
			if (w10 > 65535) {
				W10 = 65535;
			} else {
				W10 = w10;
			}

			w11 *= 65536;
			if (w11 > 65535) {
				W11 = 65535;
			} else {
				W11 = w11;
			}

			if (X >= 0 && X <= w - 2 && Y >= 0 && Y <= h - 2) {
				CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 0) = Z % 65536;
				CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 1) = Z / 65536;
				CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 2) = W00;
				CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 3) = W01;
				CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 4) = W10;
				CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 5) = W11;
			} else {
				CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 0) = 0xffff;
				CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 1) = 0xffff;
				CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 2) = 0;
				CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 3) = 0;
				CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 4) = 0;
				CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 5) = 0;
			}
		}
	}

	CvMat	*right = cvCreateMat(h, 6 * w, CV_16UC1);
	for (i = 0; i < w; i++) {
		for (j = 0; j < h; j++) {
			x = CV_MAT_ELEM(*mx2, float, j, i);
			y = CV_MAT_ELEM(*my2, float, j, i);
			X = cvFloor(x);
			Y = cvFloor(y);
			Z = Y * w + X;
			x = x - X;
			y = y - Y;
			w00 = (1 - x) * (1 - y);
			w01 = x * (1 - y);
			w10 = (1 - x) * y;
			w11 = x * y;

			w00 *= 65536;
			if (w00 > 65535) {
				W00 = 65535;
			} else {
				W00 = w00;
			}

			w01 *= 65536;
			if (w01 > 65535) {
				W01 = 65535;
			} else {
				W01 = w01;
			}

			w10 *= 65536;
			if (w10 > 65535) {
				W10 = 65535;
			} else {
				W10 = w10;
			}

			w11 *= 65536;
			if (w11 > 65535) {
				W11 = 65535;
			} else {
				W11 = w11;
			}

			if (X >= 0 && X <= w - 2 && Y >= 0 && Y <= h - 2) {
				CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 0) = Z % 65536;
				CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 1) = Z / 65536;
				CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 2) = W00;
				CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 3) = W01;
				CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 4) = W10;
				CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 5) = W11;
			} else {
				CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 0) = 0xffff;
				CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 1) = 0xffff;
				CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 2) = 0;
				CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 3) = 0;
				CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 4) = 0;
				CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 5) = 0;
			}
		}
	}

	CvMat	*ft = cvCreateMat(1, 1, CV_32SC1);
	CV_MAT_ELEM(*ft, int, 0, 0) = (int)(q[2][3] / q[3][2] + 0.5);

	unsigned int xl, xr, yl, yh;

	xl = 0;
	xr = w - 1;
	yl = 0;
	yh = h - 1;

	for (i = 0; i < w; i++) {
		for (j = h / 4; j < 3 * h / 4; j++) {
			if (CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 0) == 0xffff) {
				break;
			}
		}
		if (j >= 3 * h / 4) {
			break;
		}
	}
	if (i < w - 1 && i > xl) {
		xl = i;
	}

	for (i = w - 1; i >= 0; i--) {
		for (j = h / 4; j < 3 * h / 4; j++) {
			if (CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 0) == 0xffff) {
				break;
			}
		}
		if (j >= 3 * h / 4) {
			break;
		}
	}
	if (i >= 0 && i < xr) {
		xr = i;
	}

	for (j = 0; j < h; j++) {
		for (i = w / 4; i < 3 * w / 4; i++) {
			if (CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 0) == 0xffff) {
				break;
			}
		}
		if (i >= 3 * w / 4) {
			break;
		}
	}
	if (j < h - 1 && j > yl) {
		yl = j;
	}

	for (j = h - 1; j >= 0; j--) {
		for (i = w / 4; i < 3 * w / 4; i++) {
			if (CV_MAT_ELEM(*left, unsigned short, j, 6 * i + 0) == 0xffff) {
				break;
			}
		}
		if (i >= 3 * w / 4) {
			break;
		}
	}
	if (j >= 0 && j < yh) {
		yh = j;
	}

	printf("ROI1: x = [%d %d], y = [%d %d]\n", xl, xr, yl, yh);
	CvMat	*roi1 = cvCreateMat(1, 4, CV_32SC1);
	CV_MAT_ELEM(*roi1, int, 0, 0) = xl;
	CV_MAT_ELEM(*roi1, int, 0, 1) = xr;
	CV_MAT_ELEM(*roi1, int, 0, 2) = yl;
	CV_MAT_ELEM(*roi1, int, 0, 3) = yh;
	xl = 0;
	xr = w - 1;
	yl = 0;
	yh = h - 1;
	for (i = 0; i < w; i++) {
		for (j = h / 4; j < 3 * h / 4; j++) {
			if (CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 0) == 0xffff) {
				break;
			}
		}
		if (j >= 3 * h / 4) {
			break;
		}
	}
	if (i < w - 1 && i > xl) {
		xl = i;
	}

	for (i = w - 1; i >= 0; i--) {
		for (j = h / 4; j < 3 * h / 4; j++) {
			if (CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 0) == 0xffff) {
				break;
			}
		}
		if (j >= 3 * h / 4) {
			break;
		}
	}
	if (i >= 0 && i < xr) {
		xr = i;
	}

	for (j = 0; j < h; j++) {
		for (i = w / 4; i < 3 * w / 4; i++) {
			if (CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 0) == 0xffff) {
				break;
			}
		}
		if (i >= 3 * w / 4) {
			break;
		}
	}
	if (j < h - 1 && j > yl) {
		yl = j;
	}

	for (j = h - 1; j >= 0; j--) {
		for (i = w / 4; i < 3 * w / 4; i++) {
			if (CV_MAT_ELEM(*right, unsigned short, j, 6 * i + 0) == 0xffff) {
				break;
			}
		}
		if (i >= 3 * w / 4) {
			break;
		}
	}
	if (j >= 0 && j < yh) {
		yh = j;
	}

	printf("ROI2: x = [%d %d], y = [%d %d]\n", xl, xr, yl, yh);

	CvMat	*roi2 = cvCreateMat(1, 4, CV_32SC1);
	CV_MAT_ELEM(*roi2, int, 0, 0) = xl;
	CV_MAT_ELEM(*roi2, int, 0, 1) = xr;
	CV_MAT_ELEM(*roi2, int, 0, 2) = yl;
	CV_MAT_ELEM(*roi2, int, 0, 3) = yh;

	_cvInitUndistortRectifyMap(&M1, &D1, &R1, &P1, MX1, MY1);
	for (i = 0; i < w; i++) {
		for (j = 0; j < h; j++) {
			CV_MAT_ELEM(*imz, float, j, i) = INT_MAX;
		}
	}
	float d;
	for (i = 0; i < MULTIPLE * w; i++) {
		for (j = 0; j < MULTIPLE * h; j++) {
			x = CV_MAT_ELEM(*MX1, float, j, i);
			y = CV_MAT_ELEM(*MY1, float, j, i);
			X = cvFloor(x);
			Y = cvFloor(y);

			d = (x - X) * (x - X) + (y - Y) * (y - Y);
			if (X >= 0 && X < w && Y >= 0 && Y < h && d < CV_MAT_ELEM(*imz, float, Y, X)) {
				CV_MAT_ELEM(*imz, float, Y, X) = d;
				CV_MAT_ELEM(*imx, float, Y, X) = i / MULTIPLE;
				CV_MAT_ELEM(*imy, float, Y, X) = j / MULTIPLE;
			}

			X++;
			d = (x - X) * (x - X) + (y - Y) * (y - Y);
			if (X >= 0 && X < w && Y >= 0 && Y < h && d < CV_MAT_ELEM(*imz, float, Y, X)) {
				CV_MAT_ELEM(*imz, float, Y, X) = d;
				CV_MAT_ELEM(*imx, float, Y, X) = i / MULTIPLE;
				CV_MAT_ELEM(*imy, float, Y, X) = j / MULTIPLE;
			}

			Y++;
			d = (x - X) * (x - X) + (y - Y) * (y - Y);
			if (X >= 0 && X < w && Y >= 0 && Y < h && d < CV_MAT_ELEM(*imz, float, Y, X)) {
				CV_MAT_ELEM(*imz, float, Y, X) = d;
				CV_MAT_ELEM(*imx, float, Y, X) = i / MULTIPLE;
				CV_MAT_ELEM(*imy, float, Y, X) = j / MULTIPLE;
			}

			X--;
			d = (x - X) * (x - X) + (y - Y) * (y - Y);
			if (X >= 0 && X < w && Y >= 0 && Y < h && d < CV_MAT_ELEM(*imz, float, Y, X)) {
				CV_MAT_ELEM(*imz, float, Y, X) = d;
				CV_MAT_ELEM(*imx, float, Y, X) = i / MULTIPLE;
				CV_MAT_ELEM(*imy, float, Y, X) = j / MULTIPLE;
			}
		}
	}
	CvMat *LEFT = cvCreateMat(h, 6 * w, CV_16UC1);
	for (i = 0; i < w; i++) {
		for (j = 0; j < h; j++) {
			x = CV_MAT_ELEM(*imx, float, j, i);
			y = CV_MAT_ELEM(*imy, float, j, i);
			z = CV_MAT_ELEM(*imz, float, j, i);
			X = cvFloor(x);
			Y = cvFloor(y);
			Z = Y * w + X;
			x = x - X;
			y = y - Y;
			w00 = (1 - x) * (1 - y);
			w01 = x * (1 - y);
			w10 = (1 - x) * y;
			w11 = x * y;

			w00 *= 65536;
			if (w00 > 65535) {
				W00 = 65535;
			} else {
				W00 = w00;
			}

			w01 *= 65536;
			if (w01 > 65535) {
				W01 = 65535;
			} else {
				W01 = w01;
			}

			w10 *= 65536;
			if (w10 > 65535) {
				W10 = 65535;
			} else {
				W10 = w10;
			}

			w11 *= 65536;
			if (w11 > 65535) {
				W11 = 65535;
			} else {
				W11 = w11;
			}

			if (X >= 0 && X <= w - 2 && Y >= 0 && Y <= h - 2 && z <= 2) {
				CV_MAT_ELEM(*LEFT, unsigned short, j, 6 * i + 0) = Z % 65536;
				CV_MAT_ELEM(*LEFT, unsigned short, j, 6 * i + 1) = Z / 65536;
				CV_MAT_ELEM(*LEFT, unsigned short, j, 6 * i + 2) = W00;
				CV_MAT_ELEM(*LEFT, unsigned short, j, 6 * i + 3) = W01;
				CV_MAT_ELEM(*LEFT, unsigned short, j, 6 * i + 4) = W10;
				CV_MAT_ELEM(*LEFT, unsigned short, j, 6 * i + 5) = W11;
			} else {
				CV_MAT_ELEM(*LEFT, unsigned short, j, 6 * i + 0) = 0xffff;
				CV_MAT_ELEM(*LEFT, unsigned short, j, 6 * i + 1) = 0xffff;
				CV_MAT_ELEM(*LEFT, unsigned short, j, 6 * i + 2) = 0;
				CV_MAT_ELEM(*LEFT, unsigned short, j, 6 * i + 3) = 0;
				CV_MAT_ELEM(*LEFT, unsigned short, j, 6 * i + 4) = 0;
				CV_MAT_ELEM(*LEFT, unsigned short, j, 6 * i + 5) = 0;
			}
		}
	}

	sprintf(file, "%s/calib.dat", g_path);
	FILE *B = fopen(file, "wb");
	fwrite(&w, 4, 1, B);
	fwrite(&h, 4, 1, B);
	fwrite(ft->data.ptr, 4, 1, B);
	fwrite(roi1->data.ptr, 4, 4, B);
	fwrite(roi2->data.ptr, 4, 4, B);
	fwrite(left->data.ptr, 2, 6 * w * h, B);
	fwrite(right->data.ptr, 2, 6 * w * h, B);
	fwrite(LEFT->data.ptr, 2, 6 * w * h, B);
	fclose(B);

	gettimeofday(&tv_end, NULL);
	seconds = 1 * (tv_end.tv_sec - tv_begin.tv_sec) + (tv_end.tv_usec - tv_begin.tv_usec) / 1000000;
	printf("Time: %d s\n", seconds);

	cvReleaseMat(&mx1);
	cvReleaseMat(&my1);
	cvReleaseMat(&mx2);
	cvReleaseMat(&my2);

	return 0;
}
