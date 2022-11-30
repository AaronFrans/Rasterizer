#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle) :
			origin{ _origin },
			fovAngle{ _fovAngle }
		{
		}


		Vector3 origin{};
		float fovAngle{ 90.f };
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };


		const float near{ 0.01f };
		const float far{ 100 };

		Vector3 forward{ Vector3::UnitZ };
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};


		const float moveSpeed{ 7 };
		const float mouseMoveSpeed{ 2 };
		const float rotationSpeed{ 5 * TO_RADIANS };

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = { 0.f,0.f,0.f })
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;
		}

		void CalculateViewMatrix()
		{
			//TODO W1
			//ONB => invViewMatrix
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right);

			invViewMatrix = Matrix
			{
				right,
				up,
				forward,
				origin
			};
			//Inverse(ONB) => ViewMatrix
			viewMatrix = invViewMatrix.Inverse();

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			//TODO W2

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Camera Update Logic
			//...


			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			// if shift pressed movement * 4

			//WS for Forward-Backwards amount 
			origin += (pKeyboardState[SDL_SCANCODE_W] | pKeyboardState[SDL_SCANCODE_UP]) * moveSpeed * forward * deltaTime;
			origin += (pKeyboardState[SDL_SCANCODE_S] | pKeyboardState[SDL_SCANCODE_DOWN]) * moveSpeed * -forward * deltaTime;


			//DA for Left-Right amount				 
			origin += (pKeyboardState[SDL_SCANCODE_D] | pKeyboardState[SDL_SCANCODE_RIGHT]) * moveSpeed * right * deltaTime;
			origin += (pKeyboardState[SDL_SCANCODE_A] | pKeyboardState[SDL_SCANCODE_LEFT]) * moveSpeed * -right * deltaTime;

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			switch (mouseState)
			{
			case SDL_BUTTON_LMASK:
				origin -= mouseY * mouseMoveSpeed * forward * deltaTime;

				totalYaw += mouseX * rotationSpeed;
				break;
			case SDL_BUTTON_RMASK:
				totalYaw += mouseX * rotationSpeed;
				totalPitch -= mouseY * rotationSpeed;
				break;
			case SDL_BUTTON_X2:
				origin += mouseY * mouseMoveSpeed * deltaTime * up;
				break;
			}


			Matrix pitchMatrix{ Matrix::CreateRotationX(totalPitch * TO_RADIANS) };
			Matrix yawMatrix{ Matrix::CreateRotationY(totalYaw * TO_RADIANS) };
			Matrix rollMatrix{ Matrix::CreateRotationZ(0) };

			Matrix rotationMatrix{ pitchMatrix * yawMatrix * rollMatrix };

			forward = rotationMatrix.TransformVector(Vector3::UnitZ);
			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
