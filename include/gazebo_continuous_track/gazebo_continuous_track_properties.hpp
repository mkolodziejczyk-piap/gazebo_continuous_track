#ifndef GAZEBO_CONTINUOUS_TRACK_PROPERTIES
#define GAZEBO_CONTINUOUS_TRACK_PROPERTIES

#include <vector>

#include <gazebo/common/common.hh>
#include <gazebo/physics/physics.hh>

#include <ros/package.h>

namespace gazebo {

class ContinuousTrackProperties {

public:
  ContinuousTrackProperties(const physics::ModelPtr &_model, const sdf::ElementPtr &_sdf) {
    // assert the given sdf can be parsed as plugin property config
    AssertPluginSDF(_sdf);

    sprocket = LoadSprocket(_model, _sdf->GetElement("sprocket"));
    trajectory = LoadTrajectory(_model, _sdf->GetElement("trajectory"));
    pattern = LoadPattern(_model, _sdf->GetElement("pattern"));
  }

  virtual ~ContinuousTrackProperties() {}

  // *****************
  // public properties
  // *****************

  struct Sprocket {
    physics::JointPtr joint;
    double pitch_diameter;
  };
  Sprocket sprocket;

  struct Trajectory {
    struct Segment {
      physics::JointPtr joint;
      double end_position;
    };
    std::vector< Segment > segments;
  };
  Trajectory trajectory;

  struct Pattern {
    std::size_t elements_per_round;
    struct Element {
      std::vector< sdf::ElementPtr > collision_sdfs;
      std::vector< sdf::ElementPtr > visual_sdfs;
    };
    std::vector< Element > elements;
  };
  Pattern pattern;

private:
  // ******************
  // loading properties
  // ******************

  static Sprocket LoadSprocket(const physics::ModelPtr &_model, const sdf::ElementPtr &_sdf) {
    // format has been checked in Load(). no need to check if required elements exist.

    Sprocket sprocket;

    // [joint]
    sprocket.joint = _model->GetJoint(_sdf->GetElement("joint")->Get< std::string >());
    GZ_ASSERT(sprocket.joint,
              "Cannot find a joint with the value of [sprocket]::[joint] element in sdf");
    GZ_ASSERT(sprocket.joint->GetType() & physics::Joint::HINGE_JOINT,
              "[sprocket]::[joint] must be a rotatinal joint");

    // [pitch_diameter]
    sprocket.pitch_diameter = _sdf->GetElement("pitch_diameter")->Get< double >();

    return sprocket;
  }

  static Trajectory LoadTrajectory(const physics::ModelPtr &_model, const sdf::ElementPtr &_sdf) {
    // format has been checked in LoadProperties(). no need to check if required elements exist.

    Trajectory trajectory;

    // [segment] (multiple, +)
    for (sdf::ElementPtr segment_elem = _sdf->GetElement("segment"); segment_elem;
         segment_elem = segment_elem->GetNextElement("segment")) {
      Trajectory::Segment segment;

      // []::[joint]
      segment.joint = _model->GetJoint(segment_elem->GetElement("joint")->Get< std::string >());
      GZ_ASSERT(segment.joint, "Cannot find a joint with the value of "
                               "[trajectory]::[segment]::[joint] element in sdf");
      GZ_ASSERT(segment.joint->GetType() & physics::Joint::HINGE_JOINT ||
                    segment.joint->GetType() & physics::Joint::SLIDER_JOINT,
                "[trajectory]::[segment]::[joint] must be a rotatinal or translational joint");

      // []::[end_position]
      segment.end_position = segment_elem->GetElement("end_position")->Get< double >();
      GZ_ASSERT(segment.end_position > 0.,
                "[trajectory]::[segment]::[end_position] must be positive real number");

      trajectory.segments.push_back(segment);
    }

    return trajectory;
  }

  static Pattern LoadPattern(const physics::ModelPtr &_model, const sdf::ElementPtr &_sdf) {
    // format has been checked in LoadProperties(). no need to check if required elements exist.

    Pattern pattern;

    // [elements_per_round]
    pattern.elements_per_round = _sdf->GetElement("elements_per_round")->Get< std::size_t >();
    GZ_ASSERT(pattern.elements_per_round > 0,
              "[pattern]::[elements_per_round] must be positive intger");

    // [element] (multiple, +)
    for (sdf::ElementPtr element_elem = _sdf->GetElement("element"); element_elem;
         element_elem = element_elem->GetNextElement("element")) {
      Pattern::Element element;

      // []::[collision] (multiple, *)
      if (element_elem->HasElement("collision")) {
        for (sdf::ElementPtr collision_elem = element_elem->GetElement("collision"); collision_elem;
             collision_elem = collision_elem->GetNextElement("collision")) {
          element.collision_sdfs.push_back(collision_elem->Clone());
        }
      }

      // []::[visual] (multiple, *)
      if (element_elem->HasElement("visual")) {
        for (sdf::ElementPtr visual_elem = element_elem->GetElement("visual"); visual_elem;
             visual_elem = visual_elem->GetNextElement("visual")) {
          element.visual_sdfs.push_back(visual_elem->Clone());
        }
      }

      pattern.elements.push_back(element);
    }

    return pattern;
  }

  // **************
  // formatting sdf
  // **************

  // get a sdf element which has been initialized by the plugin format file.
  // the initialied sdf may look empty but have a format information.
  static sdf::ElementPtr InitializedPluginSDF() {
    const sdf::ElementPtr sdf(new sdf::Element());
    sdf::initFile(
        ros::package::getPath("gazebo_continuous_track") + "/sdf/continuous_track_plugin.sdf", sdf);
    return sdf;
  }

  // merge the plugin format sdf and the given sdf.
  // assert if the given sdf does not match the format
  // (ex. no required element, value type mismatch, ...).
  static void AssertPluginSDF(const sdf::ElementPtr &_sdf) {
    static const sdf::ElementPtr fmt_seed(InitializedPluginSDF());
    const sdf::ElementPtr fmt(fmt_seed->Clone());
    sdf::readString("<sdf version='" SDF_VERSION "'>" + _sdf->ToString("") + "</sdf>", fmt);
  }
};
} // namespace gazebo

#endif