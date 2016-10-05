
import collections
import gym

from q_learning import QLearning

number_of_episodes = 3000
q_learning = QLearning(list(range(500)), list(range(6)), number_of_episodes)

env = gym.make('Taxi-v1')

print(env.action_space)
print(env.observation_space)

hundred_slice = collections.deque()
hundred_counter = 1

for i_episode in range(number_of_episodes):
    observation = env.reset()

    episode_reward = 0
    for t in range(10000):
        env.render()
        print(observation)

        action = q_learning.q_e_greedy(observation, i_episode)
        old_observation = observation
        observation, reward, done, info = env.step(action)

        q_learning.value_iteration_update_ex3(old_observation, action, reward, observation)

        if done:
            episode_reward += reward
            print("Episode finished after {} timesteps".format(t+1))
            break

    # hundred_slice.append(episode_reward)
    # if hundred_counter == 100:
    #     print(sum(hundred_slice))
    #     hundred_slice.popleft()
    # else:
    #     hundred_counter += 1

    print("Episode:", i_episode, "Total reward:", episode_reward)
