#ifndef HY_INDIRECT_POSITION_HPP
#define HY_INDIRECT_POSITION_HPP

#include <HY_FlowPath.hpp>

namespace hy_features{ namespace hydrolocation{

enum class HY_DistanceDiscription
{
    at,
    between,
    downstream,
    left,
    middle,
    nearby,
    right,
    upstream,
    undefined
};

struct HY_DistanceFromReferent
{
  const double absolute;
  const double interpolative;
  const std::string accuracyStatement;
  const std::string precisionStatement;

  HY_DistanceFromReferent(double absolute = -1.0,
                          double interpolative = -1.0,
                          std::string accuracyStatement={},
                          std::string precisionStatement={}):
                          absolute(absolute), interpolative(interpolative),
                          accuracyStatement(accuracyStatement), precisionStatement(precisionStatement){
      //TODO validate values and/or combinations of values

  }

};

class HY_IndirectPosition
{
public:
  HY_IndirectPosition();
  virtual ~HY_IndirectPosition();

protected:

private:
  HY_DistanceFromReferent distance_from_referent;
  HY_DistanceDiscription distance_description;
  std::shared_ptr<HY_FlowPath> flowpath;
};
}//end hydrolocation namespace
}//end hy_features namespace
#endif //HY_INDIRECT_POSITION_HPP
