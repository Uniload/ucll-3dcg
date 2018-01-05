#include "ray-tracer-v6.h"
#include "ray-tracer-v5.h"
#include "raytracers/ray-tracers.h"

using namespace imaging;
using namespace math;
using namespace raytracer;


imaging::Color raytracer::raytracers::_private_::RayTracerV6::trace(const Scene &scene, const math::Ray &ray, double weight) const
{
	Color result = colors::black();

	if (weight > 0.01)
	{
		Hit hit;

		if (scene.root->find_first_positive_hit(ray, &hit))
		{
			MaterialProperties props = hit.material->at(hit.local_position);

			result += compute_ambient(props);
			result += process_lights(scene, props, hit, ray);
			result += compute_reflection(scene, props, ray, hit, weight);

			// Identical to v5, but calls compute_refraction additionally.
			result += compute_refraction(scene, props, ray, hit, weight);

			return result;
		}
	}
	return result;
}

imaging::Color raytracer::raytracers::_private_::RayTracerV6::compute_refraction(const Scene &scene, const MaterialProperties &props, const math::Ray &ray, const Hit &hit, double weight) const
{
	if (props.transparency > 0)
	{
		// Compute the refracted ray wtih n1 = 1 and n2 = refractive index of the material
		Ray refracted_ray = compute_refracted_ray(scene, ray, hit, 1.0, props.refractive_index);

		if (refracted_ray == ray)
		{
			// Total internal reflection occurred.
			return colors::black();
		}

		Hit exit_hit;
		
		// Find exit point
		if (scene.root->find_first_positive_hit(refracted_ray, &exit_hit))
		{
			// Compute the exit ray with n1 = refractive index of the material and n2 = 1.
			Ray exit_ray = compute_refracted_ray(scene, refracted_ray, exit_hit, props.refractive_index, 1.0);

			// Continue tracing the exit ray.
			return trace(scene, exit_ray, weight * props.transparency) * props.transparency;
		}
	}
	return colors::black();
}

math::Ray raytracer::raytracers::_private_::RayTracerV6::compute_refracted_ray(const Scene &scene, const math::Ray &ray, const Hit &hit, double n1, double n2) const
{
	// Calculate the direction of the incoming ray.
	Vector3D direction_incoming = (hit.position - ray.origin).normalized();

	Vector3D outgoing_x = (n1 / n2) * (direction_incoming - (direction_incoming.dot(hit.normal)) * hit.normal);

	if ((1 - outgoing_x.norm_sqr()) < 0)
	{
		// Total internal reflection occurred.
		return ray;
	}

	Vector3D outgoing_y = (0 - sqrt(1 - outgoing_x.norm_sqr())) * hit.normal;

	// Calculate the direction the new outgoing ray should have
	Vector3D direction_outgoing = outgoing_x + outgoing_y;

	// Calculate the origin of the new ray, adding 0.00000001 * direction_outgoing to prevent find_first_positive hit from returning P1
	Point3D outgoing_origin = ray.at(hit.t) + (0.00000001 * direction_outgoing);

	// Initialize the new refracted ray.
	Ray refracted_ray = Ray(outgoing_origin, direction_outgoing);

	return refracted_ray;
}

RayTracer raytracer::raytracers::v6()
{
	return raytracer::RayTracer(std::make_shared<raytracer::raytracers::_private_::RayTracerV6>());
}
