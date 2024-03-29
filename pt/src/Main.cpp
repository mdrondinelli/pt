#include <chrono>
#include <fstream>
#include <iostream>

#include <glm/ext.hpp>

#include "Camera.h"
#include "Film.h"
#include "Integrator.h"

int main() {
  pt::Scene scene;

  auto diffuse = std::make_shared<pt::Bxdf>(pt::DiffuseBrdf{glm::vec3{0.25f}});
  auto emissive = std::make_shared<pt::Bxdf>(pt::DiffuseBrdf{glm::vec3{0.0f}},
                                             glm::vec3{1.0f});
  auto sphereMesh = std::make_shared<pt::Mesh>("res/sphere.obj");
  auto sceneMesh = std::make_shared<pt::Mesh>("res/cornell.obj");

  auto sphereTransform =
      glm::scale(glm::identity<glm::mat4>(), glm::vec3{0.1f});
  auto spherePrimitive =
      std::make_shared<pt::Primitive>(sphereMesh, emissive, sphereTransform);

  auto sceneTransform = glm::identity<glm::mat4>();
  auto scenePrimitive =
      std::make_shared<pt::Primitive>(sceneMesh, diffuse, sceneTransform);

  scene.add(spherePrimitive);
  scene.add(scenePrimitive);
  scene.updateDeviceRepresentation();

  glm::vec3 eye{0.0f, 0.0f, 4.0f};
  glm::vec3 center{0.0f, 0.0f, 0.0f};
  glm::vec3 up{0.0f, 1.0f, 0.0f};

  auto localZ = glm::normalize(eye - center);
  auto localX = glm::normalize(glm::cross(up, localZ));
  auto localY = glm::cross(localZ, localX);
  glm::mat4 cameraTransform;
  cameraTransform[0] = glm::vec4{localX, 0.0f};
  cameraTransform[1] = glm::vec4{localY, 0.0f};
  cameraTransform[2] = glm::vec4{localZ, 0.0f};
  cameraTransform[3] = glm::vec4{eye, 1.0f};

  pt::Film film{{1920, 1080}};
  auto camera = std::make_shared<pt::Camera>(
      cameraTransform, glm::radians(45.0f),
      (float)film.getSize().x / (float)film.getSize().y);
  scene.setCamera(camera);

  pt::Integrator integrator{64 * 64 * 8 * 8};

  auto begin = std::chrono::high_resolution_clock::now();
  for (auto tileY = 0; tileY < film.getSize().y; tileY += 64) {
    auto minY = tileY;
    auto maxY = glm::min(tileY + 64, film.getSize().y);

    for (auto tileX = 0; tileX < film.getSize().x; tileX += 64) {
      auto minX = tileX;
      auto maxX = glm::min(tileX + 64, film.getSize().x);

      integrator.Li(scene, film, {minX, minY}, {maxX - minX, maxY - minY},
                    {8, 8});
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<float> duration = end - begin;
  std::cout << "Took " << duration.count() << "s\n";

  auto image = film.develop();

  std::ofstream os{"out.ppm", std::ios::binary};
  os << "P6 " << image.getSize().x << " " << image.getSize().y << " 255 ";

  for (auto y = image.getSize().y - 1; y >= 0; --y) {
    for (auto x = 0; x < image.getSize().x; ++x) {
      glm::vec3 color{image.load({x, y})};
      color = glm::pow(color, glm::vec3{1.0f / 2.2f});

      uint8_t r = 255.99f * color.r;
      uint8_t g = 255.99f * color.g;
      uint8_t b = 255.99f * color.b;
      os << r << g << b;
    }
  }

  return 0;
}