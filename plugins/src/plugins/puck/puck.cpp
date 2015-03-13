/***************************************************************************
 *  puck.cpp - Plugin to control a simulated Workpiece
 *
 *  Created: Fri Feb 20 17:15:54 2015
 *  Copyright  2015  Randolph Maaßen
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL file in the doc directory.
 */

#include <math.h>
#include <gazebo/physics/PhysicsTypes.hh>

#include "puck.h"

using namespace gazebo;

// Register this plugin to make it available in the simulator
GZ_REGISTER_MODEL_PLUGIN(Puck)

///Constructor
Puck::Puck()
{
}
///Destructor
Puck::~Puck()
{
  printf("Destructing Puck Plugin!\n");
}

/** on loading of the plugin
 * @param _parent Parent Model
 */
void Puck::Load(physics::ModelPtr _parent, sdf::ElementPtr /*_sdf*/)
{
  // Store the pointer to the model
  this->model_ = _parent;

  // get the model-name
  this->name_ = model_->GetName();
  printf("Loading Puck Plugin of model %s\n", name_.c_str());

  // Listen to the update event. This event is broadcast every
  // simulation iteration.
  this->update_connection_ = event::Events::ConnectWorldUpdateBegin(boost::bind(&Puck::OnUpdate, this, _1));

  // Create the communication Node for communication with fawkes
  this->node_ = transport::NodePtr(new transport::Node());
  // the namespace is set to the world name!
  this->node_->Init(model_->GetWorld()->GetName());

  // subscribe for puck commands
  this->command_subscriber = this->node_->Subscribe(std::string("~/") + this->name_ + std::string("/cmd"), &Puck::on_command_msg, this);
  printf("\n");

  // register visual publisher
  this->visual_pub_ = this->node_->Advertise<msgs::Visual>("~/visual");

  // initialize without rings or cap
  this->ring_count_ = 0;
  this->have_cap = false;
}

/** Called by the world update start event
 */
void Puck::OnUpdate(const common::UpdateInfo & /*_info*/)
{
}

/** on Gazebo reset
 */
void Puck::Reset()
{
}



/** Functions for recieving puck locations Messages
 * @param ring the new ring to put on top
 */ 
void Puck::on_command_msg(ConstWorkpieceCommandPtr &cmd)
{
  switch(cmd->command()){
    case gazsim_msgs::Command::ADD_RING:
      this->add_ring(cmd->color());
      break;
    case gazsim_msgs::Command::ADD_CAP:
      break;
    default:
      printf("puck %s recieved an unknowen command", this->name_.c_str());
      break;
  }

}

void Puck::add_ring(gazsim_msgs::Color clr)
{
  // create the ring name and add a new ring
  std::string ring_name = std::string("ring_") + std::to_string(this->ring_count_);
  // create a massage for visual control
  gazebo::msgs::Visual visual_msg;
  // the parent of the new visual is the workpiece itself
  visual_msg.set_parent_name(this->name_ + "::cylinder");
  // set the name of the object
  visual_msg.set_name(this->name_ + std::string("::cylinder::") + ring_name);
  // no need for shadows on the visual
  visual_msg.set_cast_shadows(false);
  // get  a geometryfor the visual
  gazebo::msgs::Geometry *geom_msg = visual_msg.mutable_geometry();
  // the geomery is roughly a cylinder
  geom_msg->set_type(msgs::Geometry::CYLINDER);
  // this model is a cylinder, so the x and y params of its bounding box
  // should be equal, the double radius. so set the radius of the addition
  // according to it
  geom_msg->mutable_cylinder()->set_radius(this->model_->GetBoundingBox().GetXLength()/2);

  // get a height, where to spawn the new visual
  double vis_middle = WORKPIECE_HEIGHT;
  std::string color_name = gazsim_msgs::Color_Name(clr);
  printf("%s has recieved a %s ring\n", this->name_.c_str(), color_name.c_str());
  // calcualte the height for the next ring
  vis_middle = (WORKPIECE_HEIGHT/2) + this->ring_count_ * RING_HEIGHT + (RING_HEIGHT/2);
  printf("vis_height is: %f\n",vis_middle);
  // the height of a ring, in meters
  geom_msg->mutable_cylinder()->set_length(RING_HEIGHT);
  //set the color according to the message
  switch(clr){
    case gazsim_msgs::Color::RED:
      msgs::Set(visual_msg.mutable_material()->mutable_diffuse(), common::Color(1,0,0));
      break;
    case gazsim_msgs::Color::BLUE:
      msgs::Set(visual_msg.mutable_material()->mutable_diffuse(), common::Color(0,0,1));
      break;
    case gazsim_msgs::Color::GREEN:
      msgs::Set(visual_msg.mutable_material()->mutable_diffuse(), common::Color(0,1,0));
      break;
    case gazsim_msgs::Color::BLACK:
    case gazsim_msgs::Color::GREY:
    default:
      msgs::Set(visual_msg.mutable_material()->mutable_diffuse(), common::Color(1,1,0));
      break;
  }
  // set the calculated pose for the visual
  msgs::Set(visual_msg.mutable_pose(),math::Pose(0,0,vis_middle,0,0,0));
  // publish visual change
  this->visual_pub_->Publish(visual_msg);
  this->ring_count_++;

}