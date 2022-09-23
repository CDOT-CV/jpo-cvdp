/** 
 * @file 
 * @author Jason M. Carter
 * @author Aaron E. Ferber
 * @date April 2017
 * @version
 *
 * @copyright Copyright 2017 US DOT - Joint Program Office
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Contributors:
 *    Oak Ridge National Laboratory.
 */

#ifndef CVDP_BSM_FILTER_H
#define CVDP_BSM_FILTER_H

#include <string>
#include <stack>
#include <vector>
#include <random>
#include "rapidjson/document.h"
#include "cvlib.hpp"
#include "redaction-properties/RedactionPropertiesManager.hpp"
#include "bsm.hpp"
#include "velocityFilter.hpp"
#include "idRedactor.hpp"

/**
 * @mainpage
 *
 * @section JPO-CVDP
 * 
 * The United States Department of Transportation Joint Program Office (JPO)
 * Connected Vehicle Data Privacy (CVDP) Project is developing a variety of methods
 * to enhance the privacy of individuals who generated connected vehicle data.
 * 
 * Connected vehicle technology uses in-vehicle wireless transceivers to broadcast
 * and receive basic safety messages (BSMs) that include accurate spatiotemporal
 * information to enhance transportation safety. Integrated Global Positioning
 * System (GPS) measurements are included in BSMs.  Databases, some publicly
 * available, of BSM sequences, called trajectories, are being used to develop
 * safety and traffic management applications. BSMs do not contain explicit
 * identifiers that link trajectories to individuals; however, the locations they
 * expose may be sensitive and associated with a very small subset of the
 * population; protecting these locations from unwanted disclosure is extremely
 * important. Developing procedures that minimize the risk of associating
 * trajectories with individuals is the objective of this project.
 * 
 * @section The Operational Data Environment (ODE) Privacy Protection Module (PPM)
 * 
 * The PPM operates on streams of raw BSMs generated by the ODE. It determines
 * whether individual BSMs should be retained or suppressed (deleted) based on the
 * information in that BSM and auxiliary map information used to define a geofence.
 * BSM geoposition (latitude and longitude) and speed are used to determine the
 * disposition of each BSM processed. The PPM also redacts other BSM fields.
 *
 * @section Configuration
 *
 * @section Operation
 */

using ConfigMap = std::unordered_map<std::string,std::string>;            ///< An alias to a string key - value configuration for the privacy parameters.

/**
 * @brief Functor class to enable use of enums as keys to unordered_maps.
 */
struct EnumHash {
    template <typename T>
        /**
         * @brief Return the hash code of a type that can be cast to a size_t, e.g., an enum.
         *
         * @return the hash code.
         */
        std::size_t operator()(T t) const 
        {
            return static_cast<std::size_t>( t );
        }
};


/** 
 * @brief A BSMHandler processes individual BSMs specified in JSON. While performing this parsing it updates (creates) a
 * BSM instance. A BSMHandler maintains state during the parsing and discontinues parsing if the BSM is determined to
 * be outside of the prescribed specification for BSM retention. Handlers are designed to be used repeatedly.  
 *
 * Currently BSMs are retained if the following conditions are met:
 *
 * - The velocity is within a specified interval [min,max].
 * - The position is within a prescribed geofence; the geofence is defined using OSM road segments.
 *
 * Currently the following BSM fields are redacted:
 *
 * - The id field is redacted for certain prescribed ids.
 *
 */
class BSMHandler {
    public:
        /**
         * records the status of the parsing including what caused parsing to stop, i.e., the point to be suppressed.
         */
        enum ResultStatus : uint16_t { SUCCESS, SPEED, GEOPOSITION, PARSE, MISSING, OTHER };

        using Ptr = std::shared_ptr<BSMHandler>;                                ///< Handle to pass this handler around efficiently.
        using ResultStringMap = std::unordered_map<ResultStatus,std::string,EnumHash>;   ///< Quick retrieval of result string.

        static ResultStringMap result_string_map;

        static constexpr uint32_t kVelocityFilterFlag = 0x1 << 0;
        static constexpr uint32_t kGeofenceFilterFlag = 0x1 << 1;
        static constexpr uint32_t kIdRedactFlag       = 0x1 << 2;
        static constexpr uint32_t kSizeRedactFlag     = 0x1 << 4;
        static constexpr uint32_t kPartIIRedactFlag   = 0x1 << 8;

        // must be static const to compose these flags and use in template specialization.
        static const unsigned flags = rapidjson::kParseDefaultFlags | rapidjson::kParseNumbersAsStringsFlag;


        /**
         * @brief Construct a BSMHandler instance using a quad tree of the map data defining the geofence and user-specified
         * configuration.
         *
         * @param quad_ptr the quad tree containing the map elements.
         * @param conf the user-specified configuration.
         */
        BSMHandler(Quad::Ptr quad_ptr, const ConfigMap& conf );

        /**
         * @brief Predicate indicating whether the BSM's position is within the prescribed geofence.
         *
         * @todo: entities use string type values; numeric types would be faster.
         *
         * @param bsm the BSM to be checked.
         * @return true if the BSM is within the geofence; false otherwise.
         */
        bool isWithinEntity(BSM &bsm) const;

        /** 
         * @brief Process a BSM presented as a JSON string; the string should not have any newlines in it.
         *
         * The result of the processing besides SAX fail/succeed status can be obtained using the #get_result method.
         *
         * @param bsm_json a JSON string of the BSM.  
         * @return true if the SAX parser did not encounter any errors during parsing; false otherwise.
         *
         */
        bool process( const std::string& bsm_json );

        /**
         * @brief Handle redacting necessary partII fields.
         *
         */
        void handlePartIIRedaction(rapidjson::Value& data);

        /**
         * @brief Recursively search for a member in a value and remove all instances of it it.
         * 
         * @param value The value to begin searching.
         * @param member The member to remove all instances of.
         * @param success The flag that the caller can check upon return to see if the operation was successful.
         */
        void findAndRemoveAllInstancesOfMember(rapidjson::Value& value, std::string member, bool& success);

        /**
         * @brief Recursively check if a member is present. Returns upon finding the first instance of the member.
         * 
         * @param value The value to begin searching.
         * @param member The member to remove.
         * @param success The flag that the caller can check upon return to see if the operation was successful.
         */
        void isMemberPresent(rapidjson::Value& value, std::string member, bool& success);

        /**
         * @brief Return the result of the most recent BSM processing.
         *
         * @return the parsing result status including success or if failure which element caused the failure.
         */
        const BSMHandler::ResultStatus get_result() const;

        /**
         * @brief Return the result of the most recent BSM processing as a string.
         *
         * @return the parsing result status including success or if failure which element caused the failure as a
         * string.
         */
        const std::string& get_result_string() const;

        /**
         * @brief Return a reference to the BSM instance generated during processing of a JSON string.
         *
         * @return a reference to the BSM instance.
         */
        BSM& get_bsm();

        /**
         * @brief Return the processed BSM as a JSON string including any changes made due to redaction of fields. This string
         * is suitable for output and does not contain any newlines.
         *
         * @return a constant reference to the processed BSM as a JSON string.
         */
        const std::string& get_json();

        /**
         * @brief Return the size in characters (bytes) of the JSON represented of the processed BSM.
         *
         * @return the size of the JSON string in chars or bytes.
         */
        std::string::size_type get_bsm_buffer_size(); 

        /**
         * @brief This method converts a variable of rapidjson::Value& type to a string.
         * 
         * @param value The rapidjson::Value& variable to convert.
         * @return string form of the rapidjson::Value& variable
         */
        std::string convertRapidjsonValueToString(rapidjson::Value& value);

        template<uint32_t FLAG>
        bool is_active() {
            return activated_ & FLAG;
        }

        template<uint32_t FLAG>
        const uint32_t activate() {
            activated_ |= FLAG;
            return activated_;
        }

        template<uint32_t FLAG>
        const uint32_t deactivate() {
            activated_ &= ~FLAG;
            return activated_;
        }

        const uint32_t get_activation_flag() const;
        const VelocityFilter& get_velocity_filter() const;
        const IdRedactor& get_id_redactor() const;

        /**
         * @brief for unit testing only.
         */
        const double get_box_extension() const;
        
    private:

        // JMC: The leak seems to be caused by re-using the RapidJSON document instance.
        // JMC: We will use a unique instance for each message.
        // rapidjson::Document document_;              ///< JSON DOM

        uint32_t activated_;                        ///< A flag word indicating which features of the privacy protection are activiated.

        bool finalized_;                            ///< Indicates the JSON string after redaction has been created and retrieved.
        ResultStatus result_;                       ///< Indicates the current state of BSM parsing and what causes failure.
        BSM bsm_;                                   ///< The BSM instance that is being built through parsing.
        Quad::Ptr quad_ptr_;                        ///< A pointer to the quad tree containing the map elements.
        bool get_value_;                            ///< Indicates the next value should be saved.
        std::string json_;                          ///< The JSON string after redaction.

        VelocityFilter vf_;                         ///< The velocity filter functor instance.
        IdRedactor idr_;                            ///< The ID Redactor to use during parsing of BSMs.

        double box_extension_;                      ///< The number of meters to extend the boxes that surround edges and define the geofence.

        RedactionPropertiesManager rpm;
};

#endif
