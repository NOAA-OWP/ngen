#include <boost/property_tree/ptree.hpp>

class MassBalanceMock {
    public:

        MassBalanceMock( bool fatal = false, double tolerance = 1e-12, int frequency = 1)
            : properties() {
            properties.put("check", true);
            properties.put("tolerance", tolerance);
            properties.put("fatal", fatal);
            properties.put("frequency", frequency);
        }

        const boost::property_tree::ptree& get() const {
            return properties;
        }
    private:
        boost::property_tree::ptree properties;
};