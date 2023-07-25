#include "GIUH.hpp"
#include "giuh_kernel.hpp"

using namespace giuh;

double giuh_kernel_impl::calc_giuh_output(double dt, double direct_runoff)
{
    // Init a running total of prior input proportional contributions to be output now
    double prior_inputs_contributions = 0.0;

    // Clean up any finished nodes from the head of the list also
    while (carry_overs_list_head != nullptr && carry_overs_list_head->last_outputted_cdf_index >= regularized_times_s.size() - 1) {
        carry_overs_list_head = carry_overs_list_head->next;
    }
    // The set a pointer for the carry over list node value to work on, starting with the head
    std::shared_ptr<giuh_carry_over> carry_over_node = carry_overs_list_head;

    // Iterate through list of carry over values, starting at the one just obtained, getting proportional output
    while (carry_over_node != nullptr) {
        // Get the index for time and regularized CDF value for getting the contribution at this time step
        // Starting from the largest, work back until the range from last index to new index is not bigger than dt
        unsigned long new_index = regularized_times_s.size() - 1;
        while ((regularized_times_s[new_index] - regularized_times_s[carry_over_node->last_outputted_cdf_index]) > dt) {
            --new_index;
        }

        // Add in the proportion of this carry-over's runoff
        double proportion = 0.0;
        for (unsigned i = carry_over_node->last_outputted_cdf_index + 1; i <= new_index; ++i) {
            proportion += interpolated_incremental_runoff[i];
        }
        prior_inputs_contributions += carry_over_node->original_input_amount * proportion;

        // Update last_outputted_cdf_index
        carry_over_node->last_outputted_cdf_index = new_index;

        // Before moving to next, prune immediately following nodes that have outputted all the original input
        while (carry_over_node->next != nullptr && carry_over_node->next->last_outputted_cdf_index >= regularized_times_s.size() - 1) {
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

    if (dt >= regularized_times_s.back()) {
        return prior_inputs_contributions + direct_runoff;
    }

    // TODO: disallow (or otherwise cleanly handle) dt arguments not divisible by interpolation_regularity_seconds
    // Get the index for time and regularized CDF value for getting the contribution at this time step
    unsigned long contribution_ordinate_index = regularized_times_s.size() - 1;
    while (regularized_times_s[contribution_ordinate_index] > dt) {
        --contribution_ordinate_index;
    }
    // Calculate ...
    double current_contribution = direct_runoff * interpolated_regularized_cdf[contribution_ordinate_index];

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

std::vector<double> giuh_kernel_impl::get_interpolated_incremental_runoff() const {
    return interpolated_incremental_runoff;
}

std::vector<int> giuh_kernel_impl::get_regularized_times_s() {
    return regularized_times_s;
}

std::vector<double> giuh_kernel_impl::get_interpolated_regularized_cdf() const {
    return interpolated_regularized_cdf;
}

/**
 * Set the object's interpolation regularity value, also triggering recalculation of the interpolated,
 * regularized CDF ordinates IFF the new regularity value is different from the previous.
 *
 * @param regularity_seconds The interpolation regularity value - i.e., the difference between each of the
 *                           corresponding times of regularized CDF ordinate - in seconds
 */
void giuh_kernel_impl::set_interpolation_regularity_seconds(unsigned int regularity_seconds) {
    if (get_interpolation_regularity_seconds() != regularity_seconds) {
        giuh_kernel::set_interpolation_regularity_seconds(regularity_seconds);
        // TODO: as with the constructor, consider setting up concurrency for this
        interpolate_regularized_cdf();
    }
}

__attribute__((no_sanitize("address")))
void giuh_kernel_impl::interpolate_regularized_cdf()
{
    // Clear any previous values
    if (!regularized_times_s.empty()) {
        regularized_times_s.clear();
    }
    if (!interpolated_regularized_cdf.empty()) {
        interpolated_regularized_cdf.clear();
    }

    // Interpolate regularized CDF (might should be done out of constructor, perhaps concurrently)
    regularized_times_s.push_back(0);
    interpolated_regularized_cdf.push_back(0);
    // Increment the ordinate time based on the regularity (loop below will do this at the end of each iter)
    unsigned int time_for_ordinate =
            regularized_times_s.back() + get_interpolation_regularity_seconds();

    // Loop through ordinate times, initializing all but the last ordinate
    while (time_for_ordinate < this->cdf_times.back()) {
        regularized_times_s.push_back(time_for_ordinate);

        // Find index 'i' of largest CDF time less than the time for the current ordinate
        // Start by getting the index of the first time greater than time_for_ordinate
        int cdf_times_index_for_iteration = 0;
        while (this->cdf_times[cdf_times_index_for_iteration] < regularized_times_s.back()) {
            cdf_times_index_for_iteration++;
        }
        // With the index of the first larger, back up one to get the last smaller
        cdf_times_index_for_iteration--;

        // Then apply equation from spreadsheet
        double result = (time_for_ordinate - this->cdf_times[cdf_times_index_for_iteration]) /
                        (this->cdf_times[cdf_times_index_for_iteration + 1] -
                         this->cdf_times[cdf_times_index_for_iteration]) *
                        (this->cdf_cumulative_freqs[cdf_times_index_for_iteration + 1] -
                         this->cdf_cumulative_freqs[cdf_times_index_for_iteration]) +
                        this->cdf_cumulative_freqs[cdf_times_index_for_iteration];
        // Push that to the back of that collection
        interpolated_regularized_cdf.push_back(result);

        // At the end of each loop iteration, increment the ordinate time based on the regularity
        time_for_ordinate = regularized_times_s.back() + get_interpolation_regularity_seconds();
    }

    // As the last step of the actual interpolation, the last ordinate time gets set to have everything
    regularized_times_s.push_back(time_for_ordinate);
    interpolated_regularized_cdf.push_back(1.0);

    // With the ordinate values interpolated, now calculate the derived, incremental values between each ordinate step
    interpolated_incremental_runoff.resize(interpolated_regularized_cdf.size());
    for (unsigned i = 0; i < interpolated_regularized_cdf.size(); i++) {
        interpolated_incremental_runoff[i] =
                i == 0 ? 0 : interpolated_regularized_cdf[i] - interpolated_regularized_cdf[i - 1];
    }
}
