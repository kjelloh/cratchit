//
// Created by kjell-olovhogdahl on 7/25/2016.
//

#ifndef TRUDELUTT_ACTIVE_H
#define TRUDELUTT_ACTIVE_H

#include <functional>
#include <thread>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace tris {
	// Active object inspired by Herb Sutter "https://herbsutter.com/2010/07/12/effective-concurrency-prefer-using-active-objects-instead-of-naked-std::threads/"
	class Active {
	public:
		typedef std::function<void()> Message;

		Active(const Active&) = delete;           // no copying
		void operator=(const Active&) = delete;    // no copying

		Active() : done(false) {
			thd = std::unique_ptr<std::thread>(
				new std::thread([=] { this->Run(); }));
		}

		~Active() {
			Send([&] { done = true; }); ;
			thd->join();
		}
		void Send(Message m) {
			mq.send(m);
		}

	private:

		template <typename Message>
		class message_queue {
		public:
			message_queue() = default;
			message_queue(const message_queue&) = delete;           // no copying
			void operator=(const message_queue&) = delete;    // no copying

			void send(Message msg) {
				// Thread locked access
				std::lock_guard<std::mutex> guard(m_Mutex);
				m_Queue.push(msg);
				m_SomethingToUnqueue.notify_one();
			}

			// Blocking receive
			Message receive() {
				// Condition variable signalled access
				std::unique_lock<std::mutex> Lock(m_Mutex);
				m_SomethingToUnqueue.wait(Lock, [this]() {return !m_Queue.empty(); }); // Wait for signalled and not empty queue
				Message result = m_Queue.front();
				m_Queue.pop();
				return result;
			}
		private:
			std::queue<Message> m_Queue;
			std::mutex m_Mutex;
			std::condition_variable m_SomethingToUnqueue;
		};


		bool done;                         // le flag
		message_queue<Message> mq;        // le queue
		std::unique_ptr<std::thread> thd;          // le std::thread

		void Run() {
			while (!done) {
				Message msg = mq.receive();
				msg();            // execute message
			} // note: last message sets done to true
		}

	};

}

#endif //TRUDELUTT_ACTIVE_H
