/*
 * This file is part of EasyRPG Player.
 *
 * EasyRPG Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EasyRPG Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasyRPG Player. If not, see <http://www.gnu.org/licenses/>.
 *
 * Based on DynRPG Particle Effects by Kazesui. (MIT license)
 */

#ifndef EP_DYNRPG_PARTICLE_H_
#define EP_DYNRPG_PARTICLE_H_

#include "game_dynrpg.h"

namespace DynRpg {
	class Particle : public DynRpgPlugin {
	public:

// Old Code - LK
//		Particle() : DynRpgPlugin("KazeParticles") {}
//		~Particle();

		Particle (Game_DynRpg& instance) : DynRpgPlugin("KazeParticles", instance) {}
        ~Particle() override;

//		void RegisterFunctions();
//		void Update();


//		void RegisterFunctions() override;

        bool Invoke(StringView func, dyn_arg_list args, bool& do_yield, Game_Interpreter* interpreter) override;

		void Update() override;
		void Load(const std::vector<uint8_t>&) override;
		std::vector<uint8_t> Save() override;


		void OnMapChange();


// Not sure if this is needed or not

//        private:
//  		bool EasyCall(dyn_arg_list args, bool& do_yield, Game_Interpreter* interpreter);
	};
}

#endif
