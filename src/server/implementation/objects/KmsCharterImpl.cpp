/* Autogenerated with kurento-module-creator */

#include <gst/gst.h>
#include "MediaPipeline.hpp"
#include <KmsCharterImplFactory.hpp>
#include "KmsCharterImpl.hpp"
#include <jsonrpc/JsonSerializer.hpp>
#include <KurentoException.hpp>

#define GST_CAT_DEFAULT kurento_kms_charter_impl
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);
#define GST_DEFAULT_NAME "KurentoKmsCharterImpl"

namespace kurento
{
namespace module
{
namespace datachannelexample
{

KmsCharterImpl::KmsCharterImpl (const boost::property_tree::ptree &config, std::shared_ptr<MediaPipeline> mediaPipeline)  : FilterImpl (config,
              std::dynamic_pointer_cast<MediaObjectImpl> ( mediaPipeline) ) 
{
  g_object_set (element, "filter-factory", "chartermetadata", NULL);

  g_object_get (G_OBJECT (element), "filter", &charter, NULL);

  if (charter == NULL) {
    throw KurentoException (MEDIA_OBJECT_NOT_AVAILABLE,
                            "Media Object Charter not available");
  }

  g_object_unref (charter);
}

MediaObjectImpl *
KmsCharterImplFactory::createObject (const boost::property_tree::ptree &config, std::shared_ptr<MediaPipeline> mediaPipeline) const
{
  return new KmsCharterImpl (config, mediaPipeline);
}

KmsCharterImpl::StaticConstructor KmsCharterImpl::staticConstructor;

KmsCharterImpl::StaticConstructor::StaticConstructor()
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0,
                           GST_DEFAULT_NAME);
}

} /* datachannelexample */
} /* module */
} /* kurento */
