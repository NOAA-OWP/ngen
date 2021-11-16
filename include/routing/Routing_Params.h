//#ifndef ROUTING_PARAMS
//#define ROUTING_PARAMS

using namespace std;

/**
 * @brief routing_params providing configuration information for routing.
 */
struct routing_params
{
    std::string t_route_config_file_with_path;

    /**
     * Default constructor, using empty strings for both member values
     */
    routing_params() : t_route_config_file_with_path("") {}

    /*
     * @brief Constructor for routing_params
     *
     * @param t_route_config_file_with_path
     */
    routing_params(std::string t_route_config_file_with_path):
        t_route_config_file_with_path(t_route_config_file_with_path)
        {
        }

};


//#endif // ROUTING_PARAMS
