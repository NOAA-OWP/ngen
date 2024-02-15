#ifndef HY_FEATURES_IDS_H
#define HY_FEATURES_IDS_H
#include <string>

namespace hy_features {
    namespace identifiers{
      /**
       * Namespace constants, define the string prefixes for various hydrofabric
       * identifier types.
      */
      static const std::string seperator = "-";
      static const std::string nexus = "nex";
      static const std::string coastal = "cnx";
      static const std::string terminal = "tnx";
      static const std::string catchment = "cat";
      static const std::string flowpath = "wb";
      static const std::string inland = "inx";

      /**
       * Determine if a provided prefix/type is representative of a Nexus concept
       * 
       * @param type string to test
      */
      inline const bool isNexus(const std::string& type){
        return (type == nexus || type == terminal || type == coastal) || type == inland;
      }

      /**
       * Determine if a provided prefix/type is representative of a Catchment concept
       * 
       * @param type string to test
      */
      inline const bool isCatchment(const std::string& type){
        //This is a next-gen specific identifier, hiding it here for now
        static const std::string aggregate = "agg";
        return (type == catchment || type == aggregate || type == flowpath);
      }

      /**
       * Determine if a provided prefix/type is representative of a Flowpath concept
       * 
       * @param type string to test
      */
      inline const bool isFlowpath(const std::string& type){
        return (type == flowpath);
      }
    }
}

#endif //HY_FEATURES_IDS_H