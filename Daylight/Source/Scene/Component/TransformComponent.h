#pragma once
#include "Component.h"

namespace Dlight
{
    class TransformComponent : public Component
    {
    public:
        TransformComponent();
        ~TransformComponent();

    public:
        void SetScale(const Vector3& s)
        {
            scale = s;
            transformDirty = true;
        }
        void SetRotation(const Quaternion& rot)
        {
            rotation = rot;
            transformDirty = true;
        }
        void SetPosition(const Vector3& pos) 
        {
            position = pos;
            transformDirty = true;
        }
        
        const Vector3& GetScale() const { return scale; };
        const Quaternion& GetRotation() const { return rotation; };
        const Vector3& GetPosition() const { return position; };

        const Matrix& GetTransformMatrix() const
        {
            if (transformDirty)
            {
                Matrix scaleMatrix = Matrix::CreateScale(scale);
                Matrix rotationMatrix = Matrix::CreateFromQuaternion(rotation);
                Matrix trnaslationMatrix = Matrix::CreateTranslation(position);
                transformMatrix = scaleMatrix * rotationMatrix * transformMatrix;
                
                transformDirty = false;
            }
            return transformMatrix;
        }

    private:
        Vector3 scale = { Vector3::One };
        Quaternion rotation = { Quaternion::Identity };
        Vector3 position = { Vector3::Zero };

        // TransformMatrix Cache 논리적 상수성을 위해 mutable
        mutable Matrix transformMatrix = { Matrix::Identity };
        mutable bool transformDirty = { true };
    };
}
