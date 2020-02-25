#ifndef HY_CATCHMENTREALIZATION_H
#define HY_CATCHMENTREALIZATION_H

#include <memory>
#include <string>

using std::shared_ptr;

class HY_Catchment;

//This is the base base for CatchmentRealizations

class HY_CatchmentRealization
{
    public:

    HY_CatchmentRealization();
    virtual ~HY_CatchmentRealization();

    protected:

    shared_ptr<HY_Catchment> realized_catchment;

    unsigned long id_number;
    std::string id;

};

#endif // HY_CATCHMENTREALIZATION_H
