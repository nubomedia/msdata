/*
 * (C) Copyright 2013 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */
#define _XOPEN_SOURCE 500

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif




#include "kmsmisc.h"
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <glib/gstdio.h>
#include <ftw.h>
#include <string.h>
#include <errno.h>


#include <gst/gst.h>
#include "commons/kmselement.h"
#include <gst/video/gstvideofilter.h>
#include <opencv/cv.h>


#if 1
#include <opencv/cv.h>
#include <opencv/highgui.h>
#else
#include <opencv2/opencv.hpp>
#include <memory>

#include <opencv2/opencv.hpp>

#endif

cv::Mat auximg;
static int foobar;

#include "kmsimageoverlaymetadata.h"
//#include "GraphUtils.h"


//#include <fstream>
//#include "gnuplot-iostream.h"

//#include <plplot.h>

//static Gnuplot gp;

#include <commons/kmsserializablemeta.h>

#define TEMP_PATH "/tmp/XXXXXX"
#define BLUE_COLOR (cvScalar (255, 0, 0, 0))
#define SRC_OVERLAY ((double)1)

#define PLUGIN_NAME "imageoverlaymetadata"

GST_DEBUG_CATEGORY_STATIC (kms_image_overlay_metadata_debug_category);
#define GST_CAT_DEFAULT kms_image_overlay_metadata_debug_category

#define KMS_IMAGE_OVERLAY_METADATA_GET_PRIVATE(obj) ( \
  G_TYPE_INSTANCE_GET_PRIVATE (              \
    (obj),                                   \
    KMS_TYPE_IMAGE_OVERLAY_METADATA,                  \
    KmsImageOverlayMetadataPrivate                   \
  )                                          \
)

enum
{
  PROP_0,
  PROP_SHOW_DEBUG_INFO
};

struct _KmsImageOverlayMetadataPrivate
{
  GstElement *text_overlay;
  IplImage *cvImage;

  gdouble offsetXPercent, offsetYPercent, widthPercent, heightPercent;
  gboolean show_debug_info;
};

typedef struct _MsMetadata{
  CvRect rect;
  int data;
  char* augmentable;
} MsMetadata;

GHashTable* myhash;
//IplImage *costumeAux;
cv::Mat costumeAux;


/* pad templates */

#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (KmsImageOverlayMetadata, kms_image_overlay_metadata,
    GST_TYPE_VIDEO_FILTER,
    GST_DEBUG_CATEGORY_INIT (kms_image_overlay_metadata_debug_category,
        PLUGIN_NAME, 0, "debug category for imageoverlay element"));


#if 0
cv::Mat set_overlay(std::string overlay_image, 
		    std::string overlay_text, 
		    float scale){
//  pthread_mutex_lock(&mMutex);
  cv::Mat fg, bg;

  if (overlay_image.length() > 0) {
    if(readImage(overlay_image, bg) == false){
      //      pthread_mutex_unlock(&mMutex);
      return cv::Mat(0, 0, CV_8UC4);
    }
  }
  if (overlay_text.length() > 0) {
    int font = cv::FONT_HERSHEY_PLAIN, font_thickness = 3;
    double font_scale = 6.0;
    int baseline=0;
    cv::Size textSize = cv::getTextSize(overlay_text.c_str(), font, font_scale, font_thickness, &baseline);
    fg = cv::Mat(textSize.height+baseline+4, textSize.width+4, CV_8UC4); // TODO: CV_8UC4
    fg.setTo(cv::Scalar(255,255,255,64));
    cv::putText(fg, overlay_text.c_str(), cv::Point(2, fg.rows-(baseline/2)), font, font_scale, cv::Scalar(0,64,0,255), font_thickness);
  }
  int res = 0;
  if (bg.data) res = std::max(bg.cols, bg.rows);
  if (fg.data) res = std::max(res, std::max(fg.cols, fg.rows));

  if (res < 1){
    //    pthread_mutex_unlock(&mMutex);
    return cv::Mat(0, 0, CV_8UC4);
  }
  cv::Mat overlay = cv::Mat(res, res, CV_8UC4);
  overlay.setTo(cv::Scalar(255,255,255,0));
  if (bg.data) {
    double fx = res/std::max(bg.cols, bg.rows), fy = fx;
    cv::resize(bg, bg, cv::Size(), fx, fy);
    cv::Mat overlay_bg(overlay, cv::Rect(
      (overlay.cols-bg.cols)/2, (overlay.rows-bg.rows)/2,
      bg.cols, bg.rows));
    bg.copyTo(overlay_bg);
  }
  if (fg.data) {
    double fx = res/std::max(fg.cols, fg.rows), fy = fx;
    cv::resize(fg, fg, cv::Size(), fx, fy);
    cv::Mat overlay_fg(overlay, cv::Rect(
      (overlay.cols-fg.cols)/2, (overlay.rows-fg.rows)/2,
      fg.cols, fg.rows));
    addFgWithAlpha(overlay_fg, fg);
  }
//  pthread_mutex_unlock(&mMutex);

  return overlay;
}
#endif

static void
kms_image_overlay_metadata_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  KmsImageOverlayMetadata *imageoverlay = KMS_IMAGE_OVERLAY_METADATA (object);

  GST_OBJECT_LOCK (imageoverlay);

  switch (property_id) {
    case PROP_SHOW_DEBUG_INFO:
      imageoverlay->priv->show_debug_info = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (imageoverlay);
}


static void
kms_image_overlay_metadata_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  KmsImageOverlayMetadata *imageoverlay = KMS_IMAGE_OVERLAY_METADATA (object);

  GST_DEBUG_OBJECT (imageoverlay, "get_property");

  GST_OBJECT_LOCK (imageoverlay);

  switch (property_id) {
    case PROP_SHOW_DEBUG_INFO:
      g_value_set_boolean (value, imageoverlay->priv->show_debug_info);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (imageoverlay);
}
#if 1
void addFgWithAlpha(cv::Mat &bg, cv::Mat &fg) {
if (fg.channels() < 4) {
fg.copyTo(bg);  
     return;
   }
   std::vector<cv::Mat> splitted_bg, splitted_fg;
   cv::split(bg, splitted_bg);
   cv::split(fg, splitted_fg);
   cv::Mat mask = splitted_fg[3];
   cv::Mat invmask = ~mask;
   splitted_bg[0] = (splitted_bg[0] - mask) + (splitted_fg[0] - invmask);
   splitted_bg[1] = (splitted_bg[1] - mask) + (splitted_fg[1] - invmask);
   splitted_bg[2] = (splitted_bg[2] - mask) + (splitted_fg[2] - invmask);
   if (bg.channels() > 3) {
     splitted_bg[3] = splitted_bg[3] + splitted_fg[3];
   }
   cv::merge(splitted_bg, bg);
 }


static void inject(cv::Mat dstMat, MsMetadata *r){
//std::cout << "INJECTING..." << std::endl;
//Gnuplot gp;

  int font = cv::FONT_HERSHEY_PLAIN;
  int font_thickness = 3;
  double font_scale = 6.0;
  int baseline=0;

  std::stringstream ss;
  ss << r->data;
  std::string txt = ""; //"JES";
  std::string tmp = ss.str();
  txt += tmp.substr(tmp.length()/2, tmp.length());

cv::Size textSize = cv::getTextSize(txt.c_str(), font, font_scale, font_thickness, &baseline);
  cv::Mat fg = cv::Mat(textSize.height+baseline+4, textSize.width+4, CV_8UC4);
  fg.setTo(cv::Scalar(255,255,255,64));
cv::putText(fg, txt.c_str(), cv::Point(2, fg.rows-(baseline/2)), font, font_scale, cv::Scalar(0,64,0,255), font_thickness);



cv::resize(fg, fg, cv::Size(dstMat.cols, dstMat.rows));
addFgWithAlpha(dstMat, fg);
  
//cv::resize(fg, fg, cv::Size(), fx, fy);

/*
if (fg.data) {
int res = 0;
double fx = res/std::max(fg.cols, fg.rows), fy = fx;

    cv::resize(fg, fg, cv::Size(), fx, fy);
    cv::Mat overlay_fg(dstMat, cv::Rect(
      (dstMat.cols-fg.cols)/2, (dstMat.rows-fg.rows)/2,
      fg.cols, fg.rows));
//addFgWithAlpha(overlay_fg, fg);
addFgWithAlpha(dstMat, fg);


  }
*/


//std::cout << "...INJECTED " << txt << std::endl;

    /*
  Gnuplot *gp = NULL;
  if(gp != NULL){
  }
    */
#if 0
Gnuplot gp;
	// Create a script which can be manually fed into gnuplot later:
	//    Gnuplot gp(">script.gp");
	// Create script and also feed to gnuplot:
	//    Gnuplot gp("tee plot.gp | gnuplot -persist");
	// Or choose any of those options at runtime by setting the GNUPLOT_IOSTREAM_CMD
	// environment variable.

	// Gnuplot vectors (i.e. arrows) require four columns: (x,y,dx,dy)
	std::vector<boost::tuple<double, double, double, double> > pts_A;

	// You can also use a separate container for each column, like so:
	std::vector<double> pts_B_x;
	std::vector<double> pts_B_y;
	std::vector<double> pts_B_dx;
	std::vector<double> pts_B_dy;

	// You could also use:
	//   std::vector<std::vector<double> >
	//   boost::tuple of four std::vector's
	//   std::vector of std::tuple (if you have C++11)
	//   arma::mat (with the Armadillo library)
	//   blitz::Array<blitz::TinyVector<double, 4>, 1> (with the Blitz++ library)
	// ... or anything of that sort

	for(double alpha=0; alpha<1; alpha+=1.0/24.0) {
		double theta = alpha*2.0*3.14159;
		pts_A.push_back(boost::make_tuple(
			 cos(theta),
			 sin(theta),
			-cos(theta)*0.1,
			-sin(theta)*0.1
		));

		pts_B_x .push_back( cos(theta)*0.8);
		pts_B_y .push_back( sin(theta)*0.8);
		pts_B_dx.push_back( sin(theta)*0.1);
		pts_B_dy.push_back(-cos(theta)*0.1);
	}

	// Don't forget to put "\n" at the end of each line!
	gp << "set xrange [-2:2]\nset yrange [-2:2]\n";
	// '-' means read from stdin.  The send1d() function sends data to gnuplot's stdin.
	gp << "plot '-' with vectors title 'pts_A', '-' with vectors title 'pts_B'\n";
	gp.send1d(pts_A);
	gp.send1d(boost::make_tuple(pts_B_x, pts_B_y, pts_B_dx, pts_B_dy));
#endif
}
#endif

static void
    kms_image_overlay_metadata_display_detections_overlay_img
    (KmsImageOverlayMetadata * imageoverlay, const GSList * faces_list)
{
const GSList *iterator = NULL;
  
/*
  if(iterator == NULL){

      MsMetadata *r = (MsMetadata*)iterator->data;
      cv::Mat roi(imageoverlay->priv->cvImage, cv::Rect(0, 0, r->rect.width, r->rect.height));
      cv::Mat dstMat = roi.clone();
      //cv::resize(costumeAux, dstMat, cv::Size(r->rect.width, r->rect.height));

inject(dstMat, r);

      dstMat.copyTo(roi);       

      cvRectangle (imageoverlay->priv->cvImage, cvPoint (0, 0),
		   cvPoint (r->rect.width, r->rect.height), cvScalar (0, 255, 255, 0), 3,
        8, 0);
    return;
  }
*/
  for (iterator = faces_list; iterator; iterator = iterator->next) {
#if 0
    CvRect *r = iterator->data;

    cvRectangle (imageoverlay->priv->cvImage, cvPoint (r->x, r->y),
		 cvPoint (r->x + r->width, r->y + r->height), cvScalar (0, 255, 255, 0), 3,
        8, 0);
#endif
    
    if(costumeAux.data == NULL){
      /*
      cvRectangle (imageoverlay->priv->cvImage, cvPoint (r->rect.x, r->rect.y),
        cvPoint (r->rect.x + r->rect.width, r->rect.y + r->rect.height), cvScalar (r->b, r->g, r->r, 0), 3,
        8, 0);
      */

      MsMetadata *r = (MsMetadata*)iterator->data;
      cv::Mat roi(imageoverlay->priv->cvImage, cv::Rect(0, 0, r->rect.width, r->rect.height));
      cv::Mat dstMat = roi.clone();
      //cv::resize(costumeAux, dstMat, cv::Size(r->rect.width, r->rect.height));

inject(dstMat, r);

      dstMat.copyTo(roi);       

      cvRectangle (imageoverlay->priv->cvImage, cvPoint (0, 0),
		   cvPoint (r->rect.width, r->rect.height), cvScalar (0, 255, 255, 0), 3,
        8, 0);
    }
    else{
      MsMetadata *r = (MsMetadata*)iterator->data;
      /*
      cvRectangle (imageoverlay->priv->cvImage, cvPoint (0, 0),
		   cvPoint (r->rect.width, r->rect.height), cvScalar (0, 255, 255, 0), 3,
        8, 0);
      */
      cv::Mat roi(imageoverlay->priv->cvImage, cv::Rect(r->rect.x, r->rect.y, r->rect.width, r->rect.height));
      cv::Mat dstMat = roi.clone();
      cv::resize(costumeAux, dstMat, cv::Size(r->rect.width, r->rect.height));

inject(dstMat, r);

      dstMat.copyTo(roi);       


      //roi = cv::Scalar(0,255,0);
      //cv::Mat mat = costumeAux;
      //dstMat = cv::Scalar(0,0,255);
      //roi.data = dstmat.data;
      //roi = dstmat;



#if 0
      cv::Mat org(imageoverlay->priv->cvImage, FALSE);
      cv::Mat mat(costumeAux, FALSE);

      cv::Size size(r->rect.width, r->rect.height);
      resize(mat, mat, size);

      cv::Mat tst(r->rect.width, r->rect.height, CV_64F);
      tst.setTo(cv::Scalar(255, 0, 0));



      /*
      cv::Mat srcRoi(mat, 
		     cv::Rect(r->rect.x, r->rect.y, 
		     		      r->rect.width, r->rect.height));
      */
      //cv::Mat dstRoi(org,
      //		     cv::Rect(r->rect.x, r->rect.y, 
      //		      r->rect.width, r->rect.height));
      //mat.copyTo(dstRoi); 
      //dstRoi.copyTo(imageoverlay->priv->cvImage); 
      IplImage *img = cvCreateImage(cvSize(r->rect.width, r->rect.height), IPL_DEPTH_8U, 4);
      //IplImage ipltemp= tst;
      //cvCopy(img, imageoverlay->priv->cvImage);
#endif
#if 0
      MsMetadata *r = (MsMetadata*)iterator->data;
      cv::Size size(r->rect.width, r->rect.height);


      cv::Mat mat = costumeAux;
      cv::Mat org = (imageoverlay->priv->cvImage);
      resize(mat, mat, size);

      cv::Mat dstRoi(org,
		     cv::Rect(r->rect.x, r->rect.y, 
			      r->rect.width, r->rect.height));
      mat.copyTo(dstRoi); 
      imageoverlay->priv->cvImage =  new IplImage(mat);
      //IplImage *image = &ipl;
      //IplImage* img = new IplImage(mat);
#endif
#if 0
      MsMetadata *r = (MsMetadata*)iterator->data;
      cvRectangle (imageoverlay->priv->cvImage, cvPoint (0, 0),
        cvPoint (r->rect.width, r->rect.height), cvScalar (r->b, r->g, r->r, 0), 3,
        8, 0);
#endif


#if 0
      MsMetadata *r = (MsMetadata*)iterator->data;

      #if 1


      //CvMat image = costumeAux;
      //cv::Mat mat;

      cv::Mat mat = costumeAux;
      cv::Mat org = imageoverlay->priv->cvImage;
      cv::Mat srcRoi(mat, 
		     cv::Rect(r->rect.x, r->rect.y, 
			      r->rect.width, r->rect.height));

      
      cv::Mat dstRoi(org,
		     cv::Rect(r->rect.x, r->rect.y, 
			      r->rect.width, r->rect.height));
      srcRoi.copyTo(dstRoi);
      //dstRoi.copyTo(imageoverlay->priv->cvImage);
 
      /*
      cvRect roi(cvPoint(r->rect.x, r->rect.y), 
	cvSize(r->rect.width, r->rect.height));
      cvMat destinationROI = imageoverlay->priv->cvImage( roi );
      costumeAux.copyTo( destinationROI );
      */
#endif

#if 0
      //addFgWithAlpha(imageoverlay->priv->cvImage, costumeAux);
      cvRectangle (imageoverlay->priv->cvImage, cvPoint (r->rect.x, r->rect.y),
        cvPoint (r->rect.x + r->rect.width, r->rect.y + r->rect.height), cvScalar (0, 0, 255, 0), 3,
        8, 0);
#endif

#if 0
      IplImage *dest = cvCloneImage(imageoverlay->priv->cvImage);
      cvNot(imageoverlay->priv->cvImage, dest);
#endif

#if 0
      int imageWidth=imageoverlay->priv->cvImage->width;
      int imageHeight=imageoverlay->priv->cvImage->height;
 cv::Mat roi(imageoverlay->priv->cvImage,cv::Rect(0,0, imageWidth, imageHeight) );
 cv::Mat sroi(mat,cv::Rect(0,0, imageWidth, imageHeight) );
sroi.copyTo(roi);
#endif

#if 0
      int imageWidth=imageoverlay->priv->cvImage->width-150;
      int imageHeight=imageoverlay->priv->cvImage->height-150;
      //int imageSize=costumeAux->nSize;
      cvRectangle(imageoverlay->priv->cvImage,cvPoint(imageWidth,imageHeight), cvPoint(50, 50),cvScalar(0, 255, 0, 0),1,8,0);
      //cvSetImageROI(imageoverlay->priv->cvImage,cvRect(50,50,(imageWidth-50),(imageHeight-50))); 
#endif
#endif
    }
  }
}

static void
kms_image_overlay_metadata_initialize_images (KmsImageOverlayMetadata *
    imageoverlay, GstVideoFrame * frame)
{
  if (imageoverlay->priv->cvImage == NULL) {
    imageoverlay->priv->cvImage =
        cvCreateImage (cvSize (frame->info.width, frame->info.height),
        IPL_DEPTH_8U, 3);

  } else if ((imageoverlay->priv->cvImage->width != frame->info.width)
      || (imageoverlay->priv->cvImage->height != frame->info.height)) {

    cvReleaseImage (&imageoverlay->priv->cvImage);
    imageoverlay->priv->cvImage =
        cvCreateImage (cvSize (frame->info.width, frame->info.height),
        IPL_DEPTH_8U, 3);
  }
}

static GSList *
receiveMetadata(GstStructure * faces)
{
  gint len, aux;
  GSList *list = NULL;

  len = gst_structure_n_fields (faces);

  for (aux = 0; aux < len; aux++) {
    GstStructure *face;
    gboolean ret;

    const gchar *name = gst_structure_nth_field_name (faces, aux);

    if (g_strcmp0 (name, "timestamp") == 0) {
      continue;
    }

    ret = gst_structure_get (faces, name, GST_TYPE_STRUCTURE, &face, NULL);

    if (ret) {
#if 1
      MsMetadata *aux = g_slice_new0 (MsMetadata);
      gst_structure_get (face, "x", G_TYPE_UINT, &aux->rect.x, NULL);
      gst_structure_get (face, "y", G_TYPE_UINT, &aux->rect.y, NULL);
      gst_structure_get (face, "width", G_TYPE_UINT, &aux->rect.width, NULL);
      gst_structure_get (face, "height", G_TYPE_UINT, &aux->rect.height, NULL);
      gst_structure_get (face, "data", G_TYPE_UINT, &aux->data, NULL);

      //GstStructure *stats;
      //gchar* remoteAddress=0;
      //g_object_get (remoteSource, "overlayimage", &stats, NULL);
      
      gst_structure_get (face, "overlay", G_TYPE_STRING, &aux->augmentable, NULL);
      if(g_hash_table_contains(myhash, aux->augmentable) == FALSE){

	std::cout << "\n kmsimageoverlay got IMAGE req: " << aux->augmentable << std::endl;
	std::string overlay = loadPlanar(aux->augmentable);
	costumeAux = cvLoadImage (overlay.c_str(), CV_LOAD_IMAGE_UNCHANGED);	
	//costumeAux = cvLoadImage (aux->augmentable, CV_LOAD_IMAGE_UNCHANGED);


	g_hash_table_add(myhash, aux->augmentable);

	std::cout << "\nTHE IMAGE: " << aux->augmentable << " " << overlay << "" <<
	  //std::cout << "\nTHE IMAGE: " << aux->augmentable << " " << "" <<
	  costumeAux.depth() << " " << costumeAux.channels() << " " << costumeAux.cols << " " << costumeAux.rows << std::endl;
      } 
      else{
      }
#if 0
      std::string msg = "POSSE:";
      msg += aux->augmentable;
	GST_STR_NULL( msg.c_str());
#endif
      //IplImage *costumeAux = cvLoadImage (aux->augmentable, CV_LOAD_IMAGE_UNCHANGED);

      gst_structure_free (face);
      list = g_slist_append (list, aux);
#endif

#if 0
      CvRect *aux = g_slice_new0 (CvRect);
      gst_structure_get (face, "x", G_TYPE_UINT, &aux->x, NULL);
      gst_structure_get (face, "y", G_TYPE_UINT, &aux->y, NULL);
      gst_structure_get (face, "width", G_TYPE_UINT, &aux->width, NULL);
      gst_structure_get (face, "height", G_TYPE_UINT, &aux->height, NULL);
      gst_structure_free (face);
      list = g_slist_append (list, aux);
#endif
    }
  }
  return list;
}

static void
cvrect_free (gpointer data)
{
#if 0
  g_slice_free (CvRect, data);
#endif
  g_slice_free (MsMetadata, data);
}


static void goVis(KmsImageOverlayMetadata *imageoverlay){

#if 0
  srand(time(NULL));
  int windowWidth = 640;
  int windowHeight = 480;
  //int memoryLength = 135;
  int memoryLength = 256;
  int randomNumber;
  std::vector<int> numbers;
  for (int i = 0; i<memoryLength; i++) {
    randomNumber = rand() % 400 + 1;
    numbers.push_back(randomNumber);
  }

  //  while (true) 
  {
    //numbers.push_back(rand() % 400 + 1);

    cv::Mat feature_vis = cv::Mat::zeros(windowHeight, windowWidth, CV_8UC3);
    int yBias = 50;
    int xBias = 50;
    char text[100];

    sprintf(text, "random");
    cv::putText(feature_vis, text, cvPoint(10, windowHeight - 15), 1, 1, CV_RGB(255, 0, 0), 1);

    sprintf(text, "0");
    cv::putText(feature_vis, text, cvPoint(10, windowHeight - yBias), 1, 1, CV_RGB(255, 0, 0), 1);
    cv::line(feature_vis, cvPoint(xBias, windowHeight - yBias), cvPoint(windowWidth - xBias, windowHeight - yBias), CV_RGB(255, 255, 255), 1);

    sprintf(text, "100");
    cv::putText(feature_vis, text, cvPoint(10, windowHeight - yBias - 100), 1, 1, CV_RGB(255, 0, 0), 1);
    cv::line(feature_vis, cvPoint(xBias, windowHeight - yBias - 100), cvPoint(windowWidth - xBias, windowHeight - yBias - 100), CV_RGB(255, 255, 255), 1);

    sprintf(text, "200");
    cv::putText(feature_vis, text, cvPoint(10, windowHeight - yBias - 200), 1, 1, CV_RGB(255, 0, 0), 1);
    cv::line(feature_vis, cvPoint(xBias, windowHeight - yBias - 200), cvPoint(windowWidth - xBias, windowHeight - yBias - 200), CV_RGB(255, 255, 255), 1);

    sprintf(text, "300");
    cv::putText(feature_vis, text, cvPoint(10, windowHeight - yBias - 300), 1, 1, CV_RGB(255, 0, 0), 1);
    cv::line(feature_vis, cvPoint(xBias, windowHeight - yBias - 300), cvPoint(windowWidth - xBias, windowHeight - yBias - 300), CV_RGB(255, 255, 255), 1);

    sprintf(text, "400");
    cv::putText(feature_vis, text, cvPoint(10, windowHeight - yBias - 400), 1, 1, CV_RGB(255, 0, 0), 1);
    cv::line(feature_vis, cvPoint(xBias, windowHeight - yBias - 400), cvPoint(windowWidth - xBias, windowHeight - yBias - 400), CV_RGB(255, 255, 255), 1);


    for (int i = 1; i<memoryLength; i++) {
      int y1 = windowHeight - yBias - numbers.at(i - 1);
      int y2 = windowHeight - yBias - numbers.at(i);
      int x1 = (windowWidth / memoryLength) * i + xBias;
      int x2 = (windowWidth / memoryLength) * (i + 1) + xBias;
      cv::line(feature_vis, cvPoint( x1, y1), cvPoint(x2, y2), CV_RGB(255, 0, 0), 2);
    }
    numbers.erase(numbers.begin());
    numbers.push_back(rand() % 400 + 1);

    cv::imshow("graph", feature_vis);

    
    if (cv::waitKey(10) > 0) {
      // Press any key to quit
      //      break;
    }
    
  }
#endif
#if 1
  //std::cout << "\njestas: " << std::endl;
  int w = 50;
  int h = 200;
  cv::Mat roi(imageoverlay->priv->cvImage, cv::Rect(0, 0, w, h));
      cv::Mat dstMat = roi.clone();
      cv::resize(auximg, dstMat, cv::Size(w, h));

      dstMat.copyTo(roi);       

      int max = 165;
      int min = 100;
      int randNum = rand()%(max-min + 1) + min;


      cvRectangle(imageoverlay->priv->cvImage, cvPoint(20, randNum), cvPoint(25, 165), cvScalar(0,0,255,255), -1);
      //      cvRectangle(imageoverlay->priv->cvImage, cvPoint(20, 100), cvPoint(25, 165), cvScalar(0,0,255,255), -1);







      //inject(dstMat, r);

      /*
      dstMat.copyTo(roi);       
      cv::Mat roi(imageoverlay->priv->cvImage, cv::Rect(r->rect.x, r->rect.y, r->rect.width, r->rect.height));
      cv::Mat dstMat = roi.clone();
      cv::resize(costumeAux, dstMat, cv::Size(r->rect.width, r->rect.height));

inject(dstMat, r);
      */
#endif

}

static GstFlowReturn
kms_image_overlay_metadata_transform_frame_ip (GstVideoFilter * filter,
    GstVideoFrame * frame)
{
  KmsImageOverlayMetadata *imageoverlay = KMS_IMAGE_OVERLAY_METADATA (filter);
  GstMapInfo info;
  KmsSerializableMeta *metadata;
  GSList *faces_list;

  gst_buffer_map (frame->buffer, &info, GST_MAP_READ);

  kms_image_overlay_metadata_initialize_images (imageoverlay, frame);
  imageoverlay->priv->cvImage->imageData = (char *) info.data;

  GST_OBJECT_LOCK (imageoverlay);
  /* check if the buffer has metadata */
  metadata = kms_buffer_get_serializable_meta (frame->buffer);

  if (metadata == NULL) {

    if(!foobar){
      foobar = !foobar;
      std::cout << "\nME IMG OVERLAY" << std::endl;
  //std::string overlay = std::string("/opt/temperature0.bmp");
  std::string overlay = loadPlanar("http://ssi.vtt.fi/temperature0.bmp");

    auximg = cvLoadImage (overlay.c_str(), CV_LOAD_IMAGE_UNCHANGED);	
    std::cout << "\nTHE IMAGE: " << overlay << " " <<
      auximg.depth() << " " << auximg.channels() << " " << auximg.cols << " " << auximg.rows << std::endl;
    }

    /*
    GstElement imageOverlay;
    GstStructure *id = NULL;

    int theid;

    std::cout << "TRY IT" << std::endl;
    g_object_get (G_OBJECT (&imageOverlay), "visid", id, NULL);
    //std::cout << "GOT imageOverlay:" << imageOverlay << std::endl;
    gst_structure_get (id, "visid", G_TYPE_UINT, &theid, NULL);
    if(id != NULL){
      std::cout << "GOT VISID:" << theid << std::endl;
    }
    else{
      std::cout << "NOTGOT VISID:" << std::endl;
    }
    */
    goVis(imageoverlay);
    goto end;
  }

  faces_list = receiveMetadata (metadata->data);

  if (faces_list != NULL) {
    kms_image_overlay_metadata_display_detections_overlay_img (imageoverlay,
        faces_list);
    g_slist_free_full (faces_list, cvrect_free);
  }

end:
  GST_OBJECT_UNLOCK (imageoverlay);

  gst_buffer_unmap (frame->buffer, &info);

  return GST_FLOW_OK;
}

static void
kms_image_overlay_metadata_dispose (GObject * object)
{
  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (kms_image_overlay_metadata_parent_class)->dispose (object);
}

static void
kms_image_overlay_metadata_finalize (GObject * object)
{
  KmsImageOverlayMetadata *imageoverlay = KMS_IMAGE_OVERLAY_METADATA (object);

  if (imageoverlay->priv->cvImage != NULL)
    cvReleaseImage (&imageoverlay->priv->cvImage);

  G_OBJECT_CLASS (kms_image_overlay_metadata_parent_class)->finalize (object);
}

#if 0
void io_new_data (GstElement* object, GstBuffer* buffer, KmsImageOverlayMetadata* self)
{
  GstMapInfo info;
  gchar *msg;

  if (!gst_buffer_map (buffer, &info, GST_MAP_READ)) {
    GST_WARNING_OBJECT (self, "Can not read buffer");
    return;
  }

  msg = g_strndup ((const gchar *) info.data, info.size);
  gst_buffer_unmap (buffer, &info);

  if (msg != NULL) {
    g_object_set (self->priv->text_overlay, "text", msg, NULL);
    g_free (msg);

  std::cout << "\njestas: " << std::endl;

  }
}


static void
kms_image_overlay_data_connect_data (KmsImageOverlayMetadata * self, GstElement * tee)
{
  GstElement *identity =  gst_element_factory_make ("identity", NULL);
  GstPad *identity_sink = gst_element_get_static_pad (identity, "sink");;

  gst_bin_add (GST_BIN (self), identity);

  kms_element_connect_sink_target (KMS_ELEMENT (self), identity_sink, KMS_ELEMENT_PAD_TYPE_DATA);
  gst_element_link (identity, tee);

  g_signal_connect (identity, "handoff", G_CALLBACK (io_new_data), self);

  g_object_unref (identity_sink);
}
#endif
static void
kms_image_overlay_metadata_init (KmsImageOverlayMetadata * imageoverlay)
{
  imageoverlay->priv = KMS_IMAGE_OVERLAY_METADATA_GET_PRIVATE (imageoverlay);

  imageoverlay->priv->show_debug_info = FALSE;
  imageoverlay->priv->cvImage = NULL;

  //  kms_image_overlay_data_connect_data (imageoverlay, kms_element_get_data_tee (KMS_ELEMENT (imageoverlay)));

}

static void
kms_image_overlay_metadata_class_init (KmsImageOverlayMetadataClass * klass)
{
  std::cout << "\n\n\nGO image_overlay_init " << std::endl;
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, PLUGIN_NAME, 0, PLUGIN_NAME);

  GST_DEBUG ("class init");

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SRC_CAPS)));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SINK_CAPS)));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "image overlay element", "Video/Filter",
      "Set a defined image in a defined position",
      "David Fernandez <d.fernandezlop@gmail.com>");

  gobject_class->set_property = kms_image_overlay_metadata_set_property;
  gobject_class->get_property = kms_image_overlay_metadata_get_property;
  gobject_class->dispose = kms_image_overlay_metadata_dispose;
  gobject_class->finalize = kms_image_overlay_metadata_finalize;

  video_filter_class->transform_frame_ip =
      GST_DEBUG_FUNCPTR (kms_image_overlay_metadata_transform_frame_ip);

  /* Properties initialization */
  g_object_class_install_property (gobject_class, PROP_SHOW_DEBUG_INFO,
      g_param_spec_boolean ("show-debug-region", "show debug region",
          "show evaluation regions over the image", FALSE, G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (KmsImageOverlayMetadataPrivate));

  myhash = g_hash_table_new(g_str_hash, g_str_equal);
  //costumeAux = NULL;
  foobar = 0;
  /*
#if 0
  //std::string overlay = std::string("/opt/temperature0.bmp");
  std::string overlay = loadPlanar("http://ssi.vtt.fi/temperature0.bmp");

    auximg = cvLoadImage (overlay.c_str(), CV_LOAD_IMAGE_UNCHANGED);	
    std::cout << "\nTHE IMAGE: " << overlay << " " <<
      auximg.depth() << " " << auximg.channels() << " " << auximg.cols << " " << auximg.rows << std::endl;
#endif
  */
    /*
      if(g_hash_table_contains(myhash, aux->augmentable) == FALSE){


	costumeAux = cvLoadImage (aux->augmentable, CV_LOAD_IMAGE_UNCHANGED);	
	g_hash_table_add(myhash, aux->augmentable);

	std::cout << "\nTHE IMAGE: " << aux->augmentable << " " <<
	  costumeAux.depth() << " " << costumeAux.channels() << " " << costumeAux.cols << " " << costumeAux.rows << std::endl;
      }
    */ 

  std::cout << "\n\n\nGO image_overlay_init sLLUT " << std::endl;
}

gboolean
kms_image_overlay_metadata_plugin_init (GstPlugin * plugin)
{
  std::cout << "\n\n\nGO image_overlay_plugin_init " << PLUGIN_NAME << std::endl;
  return gst_element_register (plugin, PLUGIN_NAME, GST_RANK_NONE,
      KMS_TYPE_IMAGE_OVERLAY_METADATA);
}
