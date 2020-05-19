#include "GIUH.hpp"

using namespace giuh;

double giuh_kernel::calc_giuh_output(double dt, double direct_runoff)
{
    // Init a running total of prior input proportional contributions to be output now
    double prior_inputs_contributions = 0.0;

    // Clean up any finished nodes from the head of the list also
    while (carry_overs_list_head != nullptr && carry_overs_list_head->last_outputted_cdf_index >= interpolated_ordinate_times_seconds.size() - 1) {
        carry_overs_list_head = carry_overs_list_head->next;
    }
    // The set a pointer for the carry over list node value to work on, starting with the head
    std::shared_ptr<giuh_carry_over> carry_over_node = carry_overs_list_head;

    // Iterate through list of carry over values, starting at the one just obtained, getting proportional output
    while (carry_over_node != nullptr) {
        // Get the index for time and regularized CDF value for getting the contribution at this time step
        // Starting from the largest, work back until the range from last index to new index is not bigger than dt
        unsigned long new_index = interpolated_ordinate_times_seconds.size() - 1;
        while ((interpolated_ordinate_times_seconds[new_index] - interpolated_ordinate_times_seconds[carry_over_node->last_outputted_cdf_index]) > dt) {
            --new_index;
        }

        // Add in the proportion of this carry-over's runoff
        double proportion = 0.0;
        for (unsigned i = carry_over_node->last_outputted_cdf_index + 1; i <= new_index; ++i) {
            proportion += interpolated_incremental_runoff_values[i];
        }
        prior_inputs_contributions += carry_over_node->original_input_amount * proportion;

        // Update last_outputted_cdf_index
        carry_over_node->last_outputted_cdf_index = new_index;

        // Before moving to next, prune immediately following nodes that have outputted all the original input
        while (carry_over_node->next != nullptr && carry_over_node->next->last_outputted_cdf_index >= interpolated_ordinate_times_seconds.size() - 1) {
            carry_over_node->next = carry_over_node->next->next;
        }

        // Move to next, though break out before actually shifting the pointer to a null value
        // (this gets and maintains the list's tail for later use)
        if (carry_over_node->next == nullptr) {
            break;
        }
        else {
            carry_over_node = carry_over_node->next;
        }
    }

    if (dt >= interpolated_ordinate_times_seconds.back()) {
        return prior_inputs_contributions + direct_runoff;
    }

    // TODO: disallow (or otherwise cleanly handle) dt arguments not divisible by interpolation_regularity_seconds
    // Get the index for time and regularized CDF value for getting the contribution at this time step
    unsigned long contribution_ordinate_index = interpolated_ordinate_times_seconds.size() - 1;
    while (interpolated_ordinate_times_seconds[contribution_ordinate_index] > dt) {
        --contribution_ordinate_index;
    }
    // Calculate ...
    double current_contribution = direct_runoff * interpolated_ordinate_times_seconds[contribution_ordinate_index];

    // If we have the list's tail, append to it; otherwise there is no current list, so start one
    if (carry_over_node != nullptr) {
        carry_over_node->next = std::make_shared<giuh_carry_over>(giuh_carry_over(direct_runoff, contribution_ordinate_index));
    }
    else {
        carry_overs_list_head = std::make_shared<giuh_carry_over>(giuh_carry_over(direct_runoff, contribution_ordinate_index));
    }
    // Return the sum of the current contribution plus contributions from prior inputs, if applicable.
    return current_contribution + prior_inputs_contributions;
}

std::string giuh_kernel::get_catchment_id()
{
    return catchment_id;
}
